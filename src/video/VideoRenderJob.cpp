#include "VideoRenderJob.h"

#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QProcess>
#include <QSet>

#include "core/zones/ColorProvider.h"
#include "core/zones/ZoneCalculator.h"
#include "maps/MapFitMath.h"
#include "maps/TileCache.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
static constexpr int kTilePx = 256;
static constexpr double kPi = 3.14159265358979323846;

QPointF latLonToTile(double lat, double lon, int zoom)
{
    const double n = std::pow(2.0, zoom);
    const double x = (lon + 180.0) / 360.0 * n;
    const double rad = lat * kPi / 180.0;
    const double y = (1.0 - std::log(std::tan(rad) + 1.0 / std::cos(rad)) / kPi) / 2.0 * n;
    return { x, y };
}

QPointF tileToScreen(const QPointF& tilePt, const QPointF& centerTile, int width, int height)
{
    return {
        width / 2.0 + (tilePt.x() - centerTile.x()) * kTilePx,
        height / 2.0 + (tilePt.y() - centerTile.y()) * kTilePx
    };
}

int nearestRecordIndex(const std::vector<RideRecord>& records, double elapsedSeconds)
{
    if (records.empty())
        return -1;

    int best = 0;
    double bestDist = std::numeric_limits<double>::max();
    for (int i = 0; i < static_cast<int>(records.size()); ++i)
    {
        const double dist = std::abs(records[static_cast<size_t>(i)].elapsedSeconds - elapsedSeconds);
        if (dist < bestDist)
        {
            bestDist = dist;
            best = i;
        }
    }
    return best;
}

bool interpolateGpsAtTime(const std::vector<const RideRecord*>& gpsRecords,
                          double elapsedSeconds,
                          double& outLat,
                          double& outLon)
{
    if (gpsRecords.empty())
        return false;

    if (elapsedSeconds <= gpsRecords.front()->elapsedSeconds)
    {
        outLat = gpsRecords.front()->latitude;
        outLon = gpsRecords.front()->longitude;
        return true;
    }

    if (elapsedSeconds >= gpsRecords.back()->elapsedSeconds)
    {
        outLat = gpsRecords.back()->latitude;
        outLon = gpsRecords.back()->longitude;
        return true;
    }

    auto it = std::lower_bound(
        gpsRecords.begin(),
        gpsRecords.end(),
        elapsedSeconds,
        [](const RideRecord* rec, double t)
        {
            return rec->elapsedSeconds < t;
        });

    if (it == gpsRecords.begin())
    {
        outLat = (*it)->latitude;
        outLon = (*it)->longitude;
        return true;
    }

    const RideRecord* b = *it;
    const RideRecord* a = *(it - 1);
    const double dt = b->elapsedSeconds - a->elapsedSeconds;
    if (dt <= 1e-6)
    {
        outLat = a->latitude;
        outLon = a->longitude;
        return true;
    }

    const double alpha = std::clamp((elapsedSeconds - a->elapsedSeconds) / dt, 0.0, 1.0);
    outLat = a->latitude + (b->latitude - a->latitude) * alpha;
    outLon = a->longitude + (b->longitude - a->longitude) * alpha;
    return true;
}

QString fmtDuration(double seconds)
{
    const int total = static_cast<int>(std::round(std::max(0.0, seconds)));
    const int h = total / 3600;
    const int m = (total % 3600) / 60;
    const int s = total % 60;
    if (h > 0)
        return QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
    return QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
}

QColor routeColor(const RideRecord& rec,
                  ColorMetric metric,
                  const ColorContext& context,
                  const QColor& singleColor)
{
    if (metric == ColorMetric::None)
        return singleColor;
    return ColorProvider::colorForMetric(metric,
                                         metric == ColorMetric::Power ? rec.power :
                                         metric == ColorMetric::HeartRate ? rec.heartRate :
                                         metric == ColorMetric::Cadence ? rec.cadence :
                                         metric == ColorMetric::Speed ? rec.speed :
                                         metric == ColorMetric::Altitude ? rec.altitude : 0.0,
                                         context);
}
}

VideoRenderJob::VideoRenderJob(VideoExportSettings settings, QObject* parent)
    : QObject(parent)
    , m_settings(std::move(settings))
{}

void VideoRenderJob::cancel()
{
    m_cancelRequested.storeRelease(1);
}

void VideoRenderJob::run()
{
    const auto canceled = [this]() { return m_cancelRequested.loadAcquire() != 0; };

    if (m_settings.rideData.records.empty())
    {
        emit finished(false, "No ride data to export.", false);
        return;
    }

    QString ffmpegPath = m_settings.ffmpegPath;
    if (ffmpegPath.isEmpty())
        ffmpegPath = QStringLiteral("ffmpeg");

    const QFileInfo outInfo(m_settings.outputPath);
    QDir().mkpath(outInfo.absolutePath());

    double startSec = std::max(0.0, m_settings.segmentStartSeconds);
    double endSec = std::max(startSec, m_settings.segmentEndSeconds);
    if (!m_settings.useSelectedSegment)
    {
        startSec = 0.0;
        endSec = m_settings.rideData.records.back().elapsedSeconds;
    }

    if (endSec <= startSec)
    {
        emit finished(false, "Selected segment is empty.", false);
        return;
    }

    const int fps = std::max(1, m_settings.fps);
    const double playbackSpeed = std::max(1.0, m_settings.playbackSpeed);
    const double sourceDuration = endSec - startSec;
    const double outputDuration = sourceDuration / playbackSpeed;
    const int totalFrames = std::max(1, static_cast<int>(std::ceil(outputDuration * fps)));

    emit stageChanged("Preparing data...");
    emit progressChanged(0, totalFrames);

    std::vector<const RideRecord*> gpsRecords;
    gpsRecords.reserve(m_settings.rideData.records.size());
    double minLat = 90.0;
    double maxLat = -90.0;
    double minLon = 180.0;
    double maxLon = -180.0;

    for (const RideRecord& rec : m_settings.rideData.records)
    {
        if (!rec.hasGps)
            continue;
        if (rec.elapsedSeconds < startSec || rec.elapsedSeconds > endSec)
            continue;

        gpsRecords.push_back(&rec);
        minLat = std::min(minLat, rec.latitude);
        maxLat = std::max(maxLat, rec.latitude);
        minLon = std::min(minLon, rec.longitude);
        maxLon = std::max(maxLon, rec.longitude);
    }

    if (gpsRecords.size() < 2)
    {
        emit finished(false, "Video export requires GPS data in the selected segment.", false);
        return;
    }

    const int mapHeight = static_cast<int>(std::round(m_settings.height * 0.72));
    const int chartHeight = std::max(120, m_settings.height - mapHeight);

    TileCache tileCache;
    tileCache.setMapStyle(m_settings.mapStyle);

    int zoom = std::clamp(m_settings.fixedZoom, 1, std::min(18, tileCache.maxZoom()));
    if (m_settings.autoZoom)
    {
        const auto fit = MapFitMath::computeFitToBounds(minLat,
                                                        maxLat,
                                                        minLon,
                                                        maxLon,
                                                        m_settings.width,
                                                        mapHeight,
                                                        std::min(18, tileCache.maxZoom()));
        zoom = fit.zoom;
    }

    emit stageChanged("Loading tiles...");

    QSet<QString> tileKeys;
    tileKeys.reserve(totalFrames * 3);
    const int preloadStep = std::max(1, totalFrames / 240);
    for (int frame = 0; frame < totalFrames; frame += preloadStep)
    {
        if (canceled())
        {
            emit finished(false, "Export canceled.", true);
            return;
        }

        const double sourceSeconds = std::min(
            endSec,
            startSec + (static_cast<double>(frame) / fps) * playbackSpeed);
        double centerLat = 0.0;
        double centerLon = 0.0;
        if (!interpolateGpsAtTime(gpsRecords, sourceSeconds, centerLat, centerLon))
            continue;

        const QPointF centerTile = latLonToTile(centerLat, centerLon, zoom);

        const int tileX0 = static_cast<int>(std::floor(centerTile.x() - static_cast<double>(m_settings.width) / (2.0 * kTilePx))) - 1;
        const int tileY0 = static_cast<int>(std::floor(centerTile.y() - static_cast<double>(mapHeight) / (2.0 * kTilePx))) - 1;
        const int nX = static_cast<int>(std::ceil(static_cast<double>(m_settings.width) / kTilePx)) + 3;
        const int nY = static_cast<int>(std::ceil(static_cast<double>(mapHeight) / kTilePx)) + 3;
        const int maxTile = (1 << zoom) - 1;

        for (int ty = 0; ty <= nY; ++ty)
        {
            for (int tx = 0; tx <= nX; ++tx)
            {
                const int tileX = tileX0 + tx;
                const int tileY = tileY0 + ty;
                if (tileX < 0 || tileY < 0 || tileX > maxTile || tileY > maxTile)
                    continue;
                tileKeys.insert(QString("%1/%2/%3").arg(zoom).arg(tileX).arg(tileY));
            }
        }
    }

    int tileIndex = 0;
    const int totalTiles = std::max(1, static_cast<int>(tileKeys.size()));
    for (const QString& tileKey : tileKeys)
    {
        if (canceled())
        {
            emit finished(false, "Export canceled.", true);
            return;
        }

        const QStringList parts = tileKey.split('/');
        if (parts.size() != 3)
            continue;

        const int z = parts[0].toInt();
        const int x = parts[1].toInt();
        const int y = parts[2].toInt();
        tileCache.tileBlocking(z, x, y, true);

        ++tileIndex;
        emit progressChanged(tileIndex, totalTiles);
    }

    emit stageChanged("Encoding video...");

    QProcess ffmpeg;
    ffmpeg.setProgram(ffmpegPath);
    ffmpeg.setArguments({
        "-y",
        "-f", "rawvideo",
        "-pixel_format", "rgba",
        "-video_size", QString("%1x%2").arg(m_settings.width).arg(m_settings.height),
        "-framerate", QString::number(fps),
        "-i", "-",
        "-an",
        "-c:v", "libx264",
        "-preset", "medium",
        "-crf", "20",
        "-pix_fmt", "yuv420p",
        m_settings.outputPath
    });
    ffmpeg.setProcessChannelMode(QProcess::MergedChannels);
    ffmpeg.start();

    if (!ffmpeg.waitForStarted())
    {
        emit finished(false, QString("Failed to start FFmpeg at '%1'.").arg(ffmpegPath), false);
        return;
    }

    static constexpr double kChartWindowBackSeconds = 180.0;
    static constexpr double kChartWindowForwardSeconds = 30.0;

    QElapsedTimer timer;
    timer.start();

    for (int frame = 0; frame < totalFrames; ++frame)
    {
        if (canceled())
        {
            ffmpeg.kill();
            ffmpeg.waitForFinished();
            emit finished(false, "Export canceled.", true);
            return;
        }

        const double sourceSeconds = std::min(
            endSec,
            startSec + (static_cast<double>(frame) / fps) * playbackSpeed);
        const int recIndex = nearestRecordIndex(m_settings.rideData.records, sourceSeconds);
        if (recIndex < 0)
            continue;

        const RideRecord& now = m_settings.rideData.records[static_cast<size_t>(recIndex)];
        double centerLat = 0.0;
        double centerLon = 0.0;
        if (!interpolateGpsAtTime(gpsRecords, sourceSeconds, centerLat, centerLon))
            continue;
        const QPointF centerTile = latLonToTile(centerLat, centerLon, zoom);

        QImage image(m_settings.width, m_settings.height, QImage::Format_RGBA8888);
        image.fill(QColor("#0f172a"));

        QPainter p(&image);
        p.setRenderHint(QPainter::Antialiasing);

        const QRect mapRect(0, 0, m_settings.width, mapHeight);
        p.fillRect(mapRect, QColor("#111827"));

        const int tileX0 = static_cast<int>(std::floor(centerTile.x() - static_cast<double>(m_settings.width) / (2.0 * kTilePx))) - 1;
        const int tileY0 = static_cast<int>(std::floor(centerTile.y() - static_cast<double>(mapHeight) / (2.0 * kTilePx))) - 1;
        const int nX = static_cast<int>(std::ceil(static_cast<double>(m_settings.width) / kTilePx)) + 3;
        const int nY = static_cast<int>(std::ceil(static_cast<double>(mapHeight) / kTilePx)) + 3;
        const int maxTile = (1 << zoom) - 1;

        for (int ty = 0; ty <= nY; ++ty)
        {
            for (int tx = 0; tx <= nX; ++tx)
            {
                const int tileX = tileX0 + tx;
                const int tileY = tileY0 + ty;
                if (tileX < 0 || tileY < 0 || tileX > maxTile || tileY > maxTile)
                    continue;

                const QPointF topLeft = tileToScreen({ static_cast<double>(tileX), static_cast<double>(tileY) },
                                                     centerTile,
                                                     m_settings.width,
                                                     mapHeight);
                const QPixmap px = tileCache.tileBlocking(zoom, tileX, tileY, false);
                if (px.isNull())
                    p.fillRect(QRectF(topLeft, QSizeF(kTilePx, kTilePx)), QColor("#334155"));
                else
                    p.drawPixmap(static_cast<int>(topLeft.x()), static_cast<int>(topLeft.y()), kTilePx, kTilePx, px);
            }
        }

        p.setPen(QPen(QColor(220, 220, 220, 110), 1.5));
        for (size_t i = 1; i < gpsRecords.size(); ++i)
        {
            const RideRecord& a = *gpsRecords[i - 1];
            const RideRecord& b = *gpsRecords[i];
            const QPointF ap = tileToScreen(latLonToTile(a.latitude, a.longitude, zoom), centerTile, m_settings.width, mapHeight);
            const QPointF bp = tileToScreen(latLonToTile(b.latitude, b.longitude, zoom), centerTile, m_settings.width, mapHeight);
            p.drawLine(ap, bp);
        }

        for (size_t i = 1; i < gpsRecords.size(); ++i)
        {
            const RideRecord& a = *gpsRecords[i - 1];
            const RideRecord& b = *gpsRecords[i];

            const QPointF ap = tileToScreen(latLonToTile(a.latitude, a.longitude, zoom), centerTile, m_settings.width, mapHeight);
            const QPointF bp = tileToScreen(latLonToTile(b.latitude, b.longitude, zoom), centerTile, m_settings.width, mapHeight);
            const QColor c = routeColor(a,
                                        m_settings.routeColorMetric,
                                        m_settings.colorContext,
                                        m_settings.singleRouteColor);
            p.setPen(QPen(c, 4.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

            if (b.elapsedSeconds <= sourceSeconds)
            {
                p.drawLine(ap, bp);
                continue;
            }

            if (a.elapsedSeconds < sourceSeconds)
            {
                const double dt = b.elapsedSeconds - a.elapsedSeconds;
                const double alpha = dt > 1e-6
                    ? std::clamp((sourceSeconds - a.elapsedSeconds) / dt, 0.0, 1.0)
                    : 0.0;
                const double lat = a.latitude + (b.latitude - a.latitude) * alpha;
                const double lon = a.longitude + (b.longitude - a.longitude) * alpha;
                const QPointF ip = tileToScreen(latLonToTile(lat, lon, zoom), centerTile, m_settings.width, mapHeight);
                p.drawLine(ap, ip);
                break;
            }

            break;
        }

        const QPointF currentPos = tileToScreen(latLonToTile(centerLat, centerLon, zoom),
                                                centerTile,
                                                m_settings.width,
                                                mapHeight);
        p.setPen(QPen(QColor("#ffffff"), 2.0));
        p.setBrush(QColor("#ef4444"));
        p.drawEllipse(currentPos, 7.0, 7.0);

        QRect telemetryRect(18, 16, 320, 180);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(15, 23, 42, 165));
        p.drawRoundedRect(telemetryRect, 8.0, 8.0);

        p.setPen(QColor("#e2e8f0"));
        p.setFont(QFont("Helvetica", 12, QFont::DemiBold));
        int y = telemetryRect.top() + 26;
        auto drawLine = [&](const QString& line)
        {
            p.drawText(telemetryRect.left() + 12, y, line);
            y += 20;
        };

        if (m_settings.overlayTime)
            drawLine(QString("Time  %1").arg(fmtDuration(sourceSeconds - startSec)));
        if (m_settings.overlayPower && now.hasPower)
            drawLine(QString("Power %1 W").arg(static_cast<int>(std::round(now.power))));
        if (m_settings.overlayHeartRate && now.hasHeartRate)
            drawLine(QString("HR    %1 bpm").arg(static_cast<int>(std::round(now.heartRate))));
        if (m_settings.overlayCadence && now.hasCadence)
            drawLine(QString("Cad   %1 rpm").arg(static_cast<int>(std::round(now.cadence))));
        if (m_settings.overlaySpeed && now.hasSpeed)
            drawLine(QString("Speed %1 km/h").arg(now.speed, 0, 'f', 1));
        if (m_settings.overlayAltitude && now.hasAltitude)
            drawLine(QString("Alt   %1 m").arg(now.altitude, 0, 'f', 0));

        if (m_settings.overlayAthleteName)
        {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(15, 23, 42, 150));
            QRect athleteRect(m_settings.width - 360, 16, 344, 42);
            p.drawRoundedRect(athleteRect, 8.0, 8.0);
            p.setPen(QColor("#f8fafc"));
            p.setFont(QFont("Helvetica", 13, QFont::Bold));
            p.drawText(athleteRect.adjusted(12, 0, -12, 0), Qt::AlignVCenter | Qt::AlignRight, m_settings.athleteName);
        }

        const QRect chartRect(0, mapHeight, m_settings.width, chartHeight);
        p.fillRect(chartRect, QColor("#020617"));

        auto drawMetricChart = [&](int row,
                                   const QString& label,
                                   auto hasMetric,
                                   auto metricValue,
                                   const QColor& color)
        {
            const int rows = 5;
            const int rowHeight = chartRect.height() / rows;
            const QRect rowRect(chartRect.left() + 12,
                                chartRect.top() + row * rowHeight + 4,
                                chartRect.width() - 24,
                                rowHeight - 8);

            p.setPen(QColor("#94a3b8"));
            p.setFont(QFont("Helvetica", 9, QFont::DemiBold));
            p.drawText(rowRect.adjusted(0, 0, 0, -rowRect.height() + 14), label);

            const double windowStart = sourceSeconds - kChartWindowBackSeconds;
            const double windowEnd = sourceSeconds + kChartWindowForwardSeconds;
            const double visibleDataStart = std::max(startSec, windowStart);
            const double visibleDataEnd = std::min(endSec, windowEnd);

            double minV = std::numeric_limits<double>::max();
            double maxV = std::numeric_limits<double>::lowest();
            std::vector<QPointF> pts;
            for (const RideRecord& rec : m_settings.rideData.records)
            {
                if (rec.elapsedSeconds < visibleDataStart || rec.elapsedSeconds > visibleDataEnd)
                    continue;
                if (!hasMetric(rec))
                    continue;
                const double v = metricValue(rec);
                minV = std::min(minV, v);
                maxV = std::max(maxV, v);
                pts.emplace_back(rec.elapsedSeconds, v);
            }

            if (pts.size() < 2)
                return;

            if (std::abs(maxV - minV) < 0.001)
                maxV = minV + 1.0;

            QPainterPath path;
            bool first = true;
            for (const QPointF& pt : pts)
            {
                const double nx = (pt.x() - windowStart) / std::max(0.001, windowEnd - windowStart);
                const double ny = (pt.y() - minV) / (maxV - minV);
                const QPointF sp(rowRect.left() + nx * rowRect.width(),
                                 rowRect.bottom() - ny * rowRect.height());
                if (first)
                {
                    path.moveTo(sp);
                    first = false;
                }
                else
                {
                    path.lineTo(sp);
                }
            }

            p.setPen(QPen(color, 1.8));
            p.drawPath(path);

            const double cursorX = rowRect.left() + ((sourceSeconds - windowStart) / std::max(0.001, windowEnd - windowStart)) * rowRect.width();
            p.setPen(QPen(QColor("#e2e8f0"), 1.0, Qt::DashLine));
            p.drawLine(QPointF(cursorX, rowRect.top()), QPointF(cursorX, rowRect.bottom()));
        };

        drawMetricChart(0, "Power", [](const RideRecord& r) { return r.hasPower; }, [](const RideRecord& r) { return r.power; }, QColor("#ef4444"));
        drawMetricChart(1, "Heart Rate", [](const RideRecord& r) { return r.hasHeartRate; }, [](const RideRecord& r) { return r.heartRate; }, QColor("#f97316"));
        drawMetricChart(2, "Cadence", [](const RideRecord& r) { return r.hasCadence; }, [](const RideRecord& r) { return r.cadence; }, QColor("#22c55e"));
        drawMetricChart(3, "Speed", [](const RideRecord& r) { return r.hasSpeed; }, [](const RideRecord& r) { return r.speed; }, QColor("#38bdf8"));
        drawMetricChart(4, "Altitude", [](const RideRecord& r) { return r.hasAltitude; }, [](const RideRecord& r) { return r.altitude; }, QColor("#c084fc"));

        p.end();

        if (ffmpeg.write(reinterpret_cast<const char*>(image.constBits()), image.sizeInBytes()) < 0)
        {
            ffmpeg.kill();
            ffmpeg.waitForFinished();
            emit finished(false, "FFmpeg pipe write failed.", false);
            return;
        }

        const qint64 elapsedMs = timer.elapsed();
        const int done = frame + 1;
        const double avgPerFrameMs = done > 0 ? static_cast<double>(elapsedMs) / static_cast<double>(done) : 0.0;
        const double remainingMs = avgPerFrameMs * static_cast<double>(totalFrames - done);
        const QString eta = fmtDuration(remainingMs / 1000.0);

        emit progressChanged(done, totalFrames);
        emit frameProgressChanged(done, totalFrames, eta);
    }

    ffmpeg.closeWriteChannel();
    if (!ffmpeg.waitForFinished(-1))
    {
        emit finished(false, "FFmpeg did not finish cleanly.", false);
        return;
    }

    if (ffmpeg.exitStatus() != QProcess::NormalExit || ffmpeg.exitCode() != 0)
    {
        const QString err = QString::fromLocal8Bit(ffmpeg.readAllStandardOutput());
        emit finished(false, QString("FFmpeg failed: %1").arg(err.left(500)), false);
        return;
    }

    emit stageChanged("Finalizing...");
    emit progressChanged(totalFrames, totalFrames);
    emit finished(true, QString("Video exported to %1").arg(m_settings.outputPath), false);
}
