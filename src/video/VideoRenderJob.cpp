// SPDX-License-Identifier: GPL-3


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
#include <QStringList>

#include "core/zones/ColorProvider.h"
#include "core/zones/ZoneCalculator.h"
#include "maps/MapFitMath.h"
#include "video/VideoTileProvider.h"
#include "video/encoding/VideoEncoder.h"
#include "video/overlay/OverlayRenderer.h"
#include "video/rendering/VideoRenderer.h"
#include "utils/FfmpegPath.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace
{
static constexpr int kTilePx = 256;
static constexpr int kVideoExportTileCache = 96;

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

class ExportTileCleanupGuard
{
public:
    explicit ExportTileCleanupGuard(VideoTileProvider& provider)
        : m_provider(provider)
    {}

    ~ExportTileCleanupGuard()
    {
        m_provider.clearMemoryCache();
        m_provider.cleanupExportCache();
    }

private:
    VideoTileProvider& m_provider;
};
}

using namespace OverlayRenderer;
using namespace VideoRenderer;

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

    QString ffmpegPath = m_settings.ffmpegPath.trimmed();
    if (ffmpegPath.isEmpty())
        ffmpegPath = resolveFfmpegExecutablePath();

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

    // If the selected segment begins before the first GPS fix, start rendering at
    // the first GPS timestamp so the route animation starts immediately.
    startSec = std::max(startSec, gpsRecords.front()->elapsedSeconds);
    if (endSec <= startSec)
    {
        emit finished(false, "Selected segment has no duration after GPS alignment.", false);
        return;
    }

    const double sourceDuration = endSec - startSec;
    const double outputDuration = sourceDuration / playbackSpeed;
    const int totalFrames = std::max(1, static_cast<int>(std::ceil(outputDuration * fps)));

    emit stageChanged("Preparing data...");
    emit progressChanged(0, totalFrames);

    const bool showAnyMetricChart = m_settings.overlayCharts &&
        (m_settings.overlayPower ||
         m_settings.overlayHeartRate ||
         m_settings.overlayCadence ||
         m_settings.overlaySpeed ||
         m_settings.overlayAltitude);
    const int mapHeight = showAnyMetricChart
        ? static_cast<int>(std::round(m_settings.height * 0.72))
        : m_settings.height;
    const int chartHeight = showAnyMetricChart
        ? std::max(120, m_settings.height - mapHeight)
        : 0;
    const double uiScale = std::clamp(static_cast<double>(m_settings.height) / 720.0, 0.85, 2.6);

    const int telemetryMargin = static_cast<int>(std::round(18.0 * uiScale));
    const int telemetryWidth = static_cast<int>(std::round(320.0 * uiScale));
    const int telemetryHeight = static_cast<int>(std::round(180.0 * uiScale));
    const int telemetryLeftPadding = static_cast<int>(std::round(12.0 * uiScale));
    const int telemetryTopBaseline = static_cast<int>(std::round(26.0 * uiScale));
    const int telemetryLineStep = static_cast<int>(std::round(20.0 * uiScale));
    const int telemetryFontPx = static_cast<int>(std::round(12.0 * uiScale));

    const int athleteWidth = static_cast<int>(std::round(344.0 * uiScale));
    const int athleteHeight = static_cast<int>(std::round(42.0 * uiScale));
    const int athleteInset = static_cast<int>(std::round(12.0 * uiScale));
    const int athleteFontPx = static_cast<int>(std::round(13.0 * uiScale));

    const int chartSidePadding = static_cast<int>(std::round(12.0 * uiScale));
    const int chartTopPadding = static_cast<int>(std::round(4.0 * uiScale));
    const int chartBottomPadding = static_cast<int>(std::round(8.0 * uiScale));
    const int chartLabelFontPx = static_cast<int>(std::round(9.0 * uiScale));
    const int chartLabelBaseline = static_cast<int>(std::round(14.0 * uiScale));

    const int legendInset = static_cast<int>(std::round(8.0 * uiScale));
    const int legendGap = static_cast<int>(std::round(8.0 * uiScale));
    const int legendSwatchSize = static_cast<int>(std::round(12.0 * uiScale));
    const int legendTitleFontPx = static_cast<int>(std::round(9.0 * uiScale));
    const int legendLabelFontPx = static_cast<int>(std::round(8.5 * uiScale));

    const std::vector<Zone> routeLegendZones = ZoneCalculator::zonesForMetric(m_settings.routeColorMetric,
                                                                               m_settings.colorContext);
    const bool showRouteLegend = m_settings.overlayRouteLegend &&
        m_settings.routeColorMetric != ColorMetric::None &&
        !routeLegendZones.empty();

    VideoTileProvider tileProvider(VideoTileProviderConfig{
        m_settings.mapStyle,
        kVideoExportTileCache,
        m_settings.deleteTemporaryTilesAfterExport,
        QString()
    });
    ExportTileCleanupGuard tileCleanupGuard(tileProvider);

    int zoom = std::clamp(m_settings.fixedZoom, 1, 18);
    if (m_settings.autoZoom)
    {
        const auto fit = MapFitMath::computeFitToBounds(minLat,
                                                        maxLat,
                                                        minLon,
                                                        maxLon,
                                                        m_settings.width,
                                                        mapHeight,
                                                        18);
        zoom = fit.zoom;
    }

    std::vector<QPointF> gpsTilePoints;
    gpsTilePoints.reserve(gpsRecords.size());
    for (const RideRecord* rec : gpsRecords)
        gpsTilePoints.push_back(latLonToTile(rec->latitude, rec->longitude, zoom));

    emit stageChanged("Analyzing required tiles...");

    std::unordered_set<VideoTileId, VideoTileIdHash> requiredTiles;
    requiredTiles.reserve(static_cast<size_t>(std::max(256, totalFrames / 3)));
    for (int frame = 0; frame < totalFrames; ++frame)
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
        const TileViewport viewport = computeTileViewport(centerTile, m_settings.width, mapHeight);
        const std::vector<VideoTileId> tiles = tilesForViewport(viewport, zoom);
        requiredTiles.insert(tiles.begin(), tiles.end());

        if (frame % 120 == 0)
            emit progressChanged(frame, totalFrames);
    }

    tileProvider.setRequiredTiles(requiredTiles);
    {
        const VideoTileStats s = tileProvider.stats();
        emit tileStatsChanged(QString("Tiles: required %1 | on-disk %2 | downloaded 0")
                                  .arg(s.requiredTiles)
                                  .arg(s.onDiskTiles));
    }
    emit stageChanged(QString("Analyzed %1 required tiles (%2 available on disk)")
                          .arg(tileProvider.requiredTileCount())
                          .arg(tileProvider.requiredTilesAvailableOnDisk()));

    emit stageChanged("Encoding video...");

    QProcess ffmpeg;
    QString ffmpegStartError;
    if (!VideoEncoder::startFfmpeg(ffmpeg, ffmpegPath, m_settings, fps, ffmpegStartError))
    {
        emit finished(false, ffmpegStartError, false);
        return;
    }

    static constexpr double kChartWindowBackSeconds = 180.0;
    static constexpr double kChartWindowForwardSeconds = 30.0;

    QElapsedTimer timer;
    timer.start();

    TileViewport lastViewport;
    bool hasLastViewport = false;
    std::vector<VideoTileId> viewportTiles;
    int currentGpsIndex = 0;

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

        const TileViewport viewport = computeTileViewport(centerTile, m_settings.width, mapHeight);
        if (!hasLastViewport || !(viewport == lastViewport))
        {
            viewportTiles = tilesForViewport(viewport, zoom);
            lastViewport = viewport;
            hasLastViewport = true;
        }

        while (currentGpsIndex + 1 < static_cast<int>(gpsRecords.size()) &&
               gpsRecords[static_cast<size_t>(currentGpsIndex + 1)]->elapsedSeconds <= sourceSeconds)
        {
            ++currentGpsIndex;
        }

        QImage image(m_settings.width, m_settings.height, QImage::Format_RGBA8888);
        image.fill(QColor("#0f172a"));

        QPainter p(&image);
        p.setRenderHint(QPainter::Antialiasing);

        const QRect mapRect(0, 0, m_settings.width, mapHeight);
        p.fillRect(mapRect, QColor("#111827"));

        for (const VideoTileId& tileId : viewportTiles)
        {
            const QPointF topLeft = tileToScreen({ static_cast<double>(tileId.x), static_cast<double>(tileId.y) },
                                                 centerTile,
                                                 m_settings.width,
                                                 mapHeight);
            const QPixmap px = tileProvider.getTile(tileId, true);
            if (px.isNull())
                p.fillRect(QRectF(topLeft, QSizeF(kTilePx, kTilePx)), QColor("#334155"));
            else
                p.drawPixmap(static_cast<int>(topLeft.x()), static_cast<int>(topLeft.y()), kTilePx, kTilePx, px);
        }

        p.setPen(QPen(QColor(220, 220, 220, 110), 1.5));
        for (size_t i = 1; i < gpsTilePoints.size(); ++i)
        {
            const QPointF ap = tileToScreen(gpsTilePoints[i - 1], centerTile, m_settings.width, mapHeight);
            const QPointF bp = tileToScreen(gpsTilePoints[i], centerTile, m_settings.width, mapHeight);
            p.drawLine(ap, bp);
        }

        const size_t maxColoredSegment = static_cast<size_t>(std::clamp(currentGpsIndex + 1, 1, static_cast<int>(gpsRecords.size()) - 1));
        for (size_t i = 1; i <= maxColoredSegment; ++i)
        {
            const RideRecord& a = *gpsRecords[i - 1];
            const QPointF ap = tileToScreen(gpsTilePoints[i - 1], centerTile, m_settings.width, mapHeight);
            const QPointF bp = tileToScreen(gpsTilePoints[i], centerTile, m_settings.width, mapHeight);
            const QColor c = routeColor(a,
                                        m_settings.routeColorMetric,
                                        m_settings.colorContext,
                                        m_settings.singleRouteColor);
            p.setPen(QPen(c, 4.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

            p.drawLine(ap, bp);
        }

        if (currentGpsIndex + 1 < static_cast<int>(gpsRecords.size()))
        {
            const RideRecord& a = *gpsRecords[static_cast<size_t>(currentGpsIndex)];
            const RideRecord& b = *gpsRecords[static_cast<size_t>(currentGpsIndex + 1)];
            if (sourceSeconds > a.elapsedSeconds && sourceSeconds < b.elapsedSeconds)
            {
                const double dt = b.elapsedSeconds - a.elapsedSeconds;
                const double alpha = dt > 1e-6
                    ? std::clamp((sourceSeconds - a.elapsedSeconds) / dt, 0.0, 1.0)
                    : 0.0;
                const QPointF ap = tileToScreen(gpsTilePoints[static_cast<size_t>(currentGpsIndex)],
                                                centerTile,
                                                m_settings.width,
                                                mapHeight);
                const QPointF bp = tileToScreen(gpsTilePoints[static_cast<size_t>(currentGpsIndex + 1)],
                                                centerTile,
                                                m_settings.width,
                                                mapHeight);
                const QPointF ip = ap + (bp - ap) * alpha;
                const QColor c = routeColor(a,
                                            m_settings.routeColorMetric,
                                            m_settings.colorContext,
                                            m_settings.singleRouteColor);
                p.setPen(QPen(c, 4.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                p.drawLine(ap, ip);
            }
        }

        const QPointF currentPos = tileToScreen(latLonToTile(centerLat, centerLon, zoom),
                                                centerTile,
                                                m_settings.width,
                                                mapHeight);
        p.setPen(QPen(QColor("#ffffff"), 2.0));
        p.setBrush(QColor("#ef4444"));
        p.drawEllipse(currentPos, 7.0, 7.0);

        if (showRouteLegend)
        {
            const bool hasAthleteBox = m_settings.overlayAthleteName;
            const int reservedLeft = telemetryMargin + telemetryWidth + telemetryMargin;
            const int reservedRight = hasAthleteBox
                ? (athleteWidth + telemetryMargin + telemetryMargin)
                : telemetryMargin;

            int legendLeft = reservedLeft;
            int legendRight = m_settings.width - reservedRight;
            int legendY = telemetryMargin;

            if (legendRight - legendLeft < static_cast<int>(std::round(260.0 * uiScale)))
            {
                legendLeft = telemetryMargin;
                legendRight = m_settings.width - telemetryMargin;
                legendY = telemetryMargin + std::max(telemetryHeight, hasAthleteBox ? athleteHeight : 0) + legendGap;
            }

            const QString legendTitle = QString("Track Color: %1").arg(colorMetricDisplayName(m_settings.routeColorMetric));
            p.setFont(QFont("Helvetica", legendTitleFontPx, QFont::DemiBold));
            const QFontMetrics titleFm = p.fontMetrics();

            p.setFont(QFont("Helvetica", legendLabelFontPx, QFont::Normal));
            const QFontMetrics labelFm = p.fontMetrics();

            const int legendWidth = std::max(120, legendRight - legendLeft);
            const int contentWidth = std::max(80, legendWidth - (2 * legendInset));
            const int rowHeight = std::max(legendSwatchSize + 2, labelFm.height() + 2);

            struct LegendLayoutItem
            {
                const Zone* zone = nullptr;
                QString label;
                int row = 0;
                int x = 0;
                int textWidth = 0;
            };

            std::vector<LegendLayoutItem> layoutItems;
            layoutItems.reserve(routeLegendZones.size());
            int currentRow = 0;
            int currentX = 0;
            for (const Zone& zone : routeLegendZones)
            {
                const QString label = zoneLegendText(zone, m_settings.routeColorMetric);
                const int textW = labelFm.horizontalAdvance(label);
                const int itemW = legendSwatchSize + legendGap + textW + legendInset;

                if (currentX > 0 && currentX + itemW > contentWidth)
                {
                    ++currentRow;
                    currentX = 0;
                }

                layoutItems.push_back({ &zone, label, currentRow, currentX, textW });
                currentX += itemW;
            }

            const int rows = std::max(1, currentRow + 1);
            const int titleHeight = titleFm.height();
            const int legendHeight = (2 * legendInset) + titleHeight + legendGap + (rows * rowHeight);

            QRect legendRect(legendLeft,
                             legendY,
                             legendWidth,
                             legendHeight);

            p.setPen(Qt::NoPen);
            p.setBrush(QColor(15, 23, 42, 170));
            p.drawRoundedRect(legendRect, 8.0, 8.0);

            p.setPen(QColor("#cbd5e1"));
            p.setFont(QFont("Helvetica", legendTitleFontPx, QFont::DemiBold));
            const QRect titleRect(legendRect.left() + legendInset,
                                  legendRect.top() + legendInset,
                                  legendRect.width() - (2 * legendInset),
                                  titleHeight);
            p.drawText(titleRect, Qt::AlignVCenter | Qt::AlignLeft, legendTitle);

            p.setFont(QFont("Helvetica", legendLabelFontPx, QFont::Normal));
            for (const LegendLayoutItem& item : layoutItems)
            {
                const int rowTop = titleRect.bottom() + legendGap + (item.row * rowHeight);
                QRect swatchRect(legendRect.left() + legendInset + item.x,
                                 rowTop + (rowHeight - legendSwatchSize) / 2,
                                 legendSwatchSize,
                                 legendSwatchSize);
                p.setPen(QPen(QColor(255, 255, 255, 90), 1.0));
                p.setBrush(item.zone->color);
                p.drawRoundedRect(swatchRect, 2.0, 2.0);

                p.setPen(QColor("#f1f5f9"));
                p.drawText(QRect(swatchRect.right() + legendGap,
                                 rowTop,
                                 item.textWidth,
                                 rowHeight),
                           Qt::AlignVCenter | Qt::AlignLeft,
                           item.label);
            }
        }

        const bool hasTelemetryValue = m_settings.overlayTime ||
            (m_settings.overlayPower && now.hasPower) ||
            (m_settings.overlayHeartRate && now.hasHeartRate) ||
            (m_settings.overlayCadence && now.hasCadence) ||
            (m_settings.overlaySpeed && now.hasSpeed) ||
            (m_settings.overlayAltitude && now.hasAltitude);

        if (m_settings.overlayTelemetryPanel && hasTelemetryValue)
        {
            QRect telemetryRect(telemetryMargin, telemetryMargin, telemetryWidth, telemetryHeight);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(15, 23, 42, 165));
            p.drawRoundedRect(telemetryRect, 8.0, 8.0);

            p.setPen(QColor("#e2e8f0"));
            p.setFont(QFont("Helvetica", telemetryFontPx, QFont::DemiBold));
            int y = telemetryRect.top() + telemetryTopBaseline;
            auto drawLine = [&](const QString& line)
            {
                p.drawText(telemetryRect.left() + telemetryLeftPadding, y, line);
                y += telemetryLineStep;
            };

            if (m_settings.overlayTime)
                drawLine(QString("Time  %1").arg(formatDuration(sourceSeconds - startSec)));
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
        }

        if (m_settings.overlayAthleteName)
        {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(15, 23, 42, 150));
            QRect athleteRect(m_settings.width - athleteWidth - telemetryMargin,
                              telemetryMargin,
                              athleteWidth,
                              athleteHeight);
            p.drawRoundedRect(athleteRect, 8.0, 8.0);
            p.setPen(QColor("#f8fafc"));
            p.setFont(QFont("Helvetica", athleteFontPx, QFont::Bold));
            p.drawText(athleteRect.adjusted(athleteInset, 0, -athleteInset, 0), Qt::AlignVCenter | Qt::AlignRight, m_settings.athleteName);
        }

        if (showAnyMetricChart)
        {
            const QRect chartRect(0, mapHeight, m_settings.width, chartHeight);
            p.fillRect(chartRect, QColor("#020617"));

            auto drawMetricChart = [&](int row,
                                       int rows,
                                       const QString& label,
                                       auto hasMetric,
                                       auto metricValue,
                                       const QColor& color)
            {
                const int rowHeight = chartRect.height() / std::max(1, rows);
            const QRect rowRect(chartRect.left() + chartSidePadding,
                                chartRect.top() + row * rowHeight + chartTopPadding,
                                chartRect.width() - (2 * chartSidePadding),
                                rowHeight - chartBottomPadding);

            p.setPen(QColor("#94a3b8"));
            p.setFont(QFont("Helvetica", chartLabelFontPx, QFont::DemiBold));
            p.drawText(rowRect.adjusted(0, 0, 0, -rowRect.height() + chartLabelBaseline), label);

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

                        struct ChartSpec
                        {
                                bool enabled;
                                QString label;
                                std::function<bool(const RideRecord&)> hasMetric;
                                std::function<double(const RideRecord&)> metricValue;
                                QColor color;
                        };

                        const std::vector<ChartSpec> chartSpecs = {
                                { m_settings.overlayPower,
                                    "Power",
                                    [](const RideRecord& r) { return r.hasPower; },
                                    [](const RideRecord& r) { return r.power; },
                                    QColor("#ef4444") },
                                { m_settings.overlayHeartRate,
                                    "Heart Rate",
                                    [](const RideRecord& r) { return r.hasHeartRate; },
                                    [](const RideRecord& r) { return r.heartRate; },
                                    QColor("#f97316") },
                                { m_settings.overlayCadence,
                                    "Cadence",
                                    [](const RideRecord& r) { return r.hasCadence; },
                                    [](const RideRecord& r) { return r.cadence; },
                                    QColor("#22c55e") },
                                { m_settings.overlaySpeed,
                                    "Speed",
                                    [](const RideRecord& r) { return r.hasSpeed; },
                                    [](const RideRecord& r) { return r.speed; },
                                    QColor("#38bdf8") },
                                { m_settings.overlayAltitude,
                                    "Altitude",
                                    [](const RideRecord& r) { return r.hasAltitude; },
                                    [](const RideRecord& r) { return r.altitude; },
                                    QColor("#c084fc") }
                        };

                        int enabledCount = 0;
                        for (const ChartSpec& spec : chartSpecs)
                        {
                                if (spec.enabled)
                                        ++enabledCount;
                        }

                        int row = 0;
                        for (const ChartSpec& spec : chartSpecs)
                        {
                                if (!spec.enabled)
                                        continue;
                                drawMetricChart(row,
                                                                enabledCount,
                                                                spec.label,
                                                                spec.hasMetric,
                                                                spec.metricValue,
                                                                spec.color);
                                ++row;
                        }
                }

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
        const QString eta = formatDuration(remainingMs / 1000.0);

        if (done == 1 || done % 60 == 0 || done == totalFrames)
        {
            const VideoTileStats s = tileProvider.stats();
            emit tileStatsChanged(QString("Tiles: required %1 | on-disk %2 | downloaded %3")
                                      .arg(s.requiredTiles)
                                      .arg(s.onDiskTiles)
                                      .arg(s.downloadedTiles));
        }

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
    {
        const VideoTileStats s = tileProvider.stats();
        emit tileStatsChanged(QString("Tiles: required %1 | on-disk %2 | downloaded %3")
                                  .arg(s.requiredTiles)
                                  .arg(s.onDiskTiles)
                                  .arg(s.downloadedTiles));
        emit finished(
            true,
            QString("Video exported to %1\n\nTile stats: required %2, on-disk %3, downloaded %4")
                .arg(m_settings.outputPath)
                .arg(s.requiredTiles)
                .arg(s.onDiskTiles)
                .arg(s.downloadedTiles),
            false);
    }
}