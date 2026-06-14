#include "MapRenderer.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>

#include "core/zones/ColorProvider.h"
#include "maps/MapFitMath.h"

#include <algorithm>
#include <cmath>
#include <limits>

static QColor gradientColorForSlope(double percent)
{
    if (percent <= -8.0) return QColor("#7c2d12");
    if (percent <= -4.0) return QColor("#ea580c");
    if (percent <= -1.0) return QColor("#f59e0b");
    if (percent < 1.0)    return QColor("#94a3b8");
    if (percent < 4.0)    return QColor("#22c55e");
    if (percent < 8.0)    return QColor("#16a34a");
    return QColor("#15803d");
}

static ColorMetric routeModeToMetric(RouteColorMode mode)
{
    switch (mode)
    {
        case RouteColorMode::Power:     return ColorMetric::Power;
        case RouteColorMode::HeartRate: return ColorMetric::HeartRate;
        case RouteColorMode::Cadence:   return ColorMetric::Cadence;
        case RouteColorMode::Speed:     return ColorMetric::Speed;
        case RouteColorMode::Altitude:  return ColorMetric::Altitude;
        case RouteColorMode::None:
        case RouteColorMode::Gradient:
            return ColorMetric::None;
    }

    return ColorMetric::None;
}

static constexpr double kPi     = 3.14159265358979323846;
static constexpr int    kTilePx = 256;

QPointF MapRenderer::latLonToTile(double lat, double lon, int zoom)
{
    const double n   = std::pow(2.0, zoom);
    const double x   = (lon + 180.0) / 360.0 * n;
    const double rad = lat * kPi / 180.0;
    const double y   = (1.0 - std::log(
        std::tan(rad) + 1.0 / std::cos(rad)) / kPi) / 2.0 * n;
    return { x, y };
}

QPointF MapRenderer::tileToScreen(const QPointF& tilePt) const
{
    const auto centre = latLonToTile(m_centerLat, m_centerLon, m_zoom);
    return {
        width()  / 2.0 + (tilePt.x() - centre.x()) * kTilePx,
        height() / 2.0 + (tilePt.y() - centre.y()) * kTilePx
    };
}

MapRenderer::MapRenderer(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(300);
    connect(&m_tileCache, &TileCache::tileLoaded,
            this, [this](int, int, int) { update(); });
}

void MapRenderer::setRideData(const RideData& rideData,
                              ColorMetric colorMetric,
                              const ColorContext& colorContext)
{
    m_rideData = rideData;
    m_hasGps   = false;
    m_userMovedMap = false;
    m_autoFitVisibleRange = true;
    m_visibleStartSeconds = -1.0;
    m_visibleEndSeconds = -1.0;
    m_currentTimeSeconds = -1.0;
    m_highlightElapsedSeconds = -1.0;
    m_routeColorMode = colorMetric == ColorMetric::None
        ? RouteColorMode::None
        : colorMetric == ColorMetric::Power
            ? RouteColorMode::Power
            : colorMetric == ColorMetric::HeartRate
                ? RouteColorMode::HeartRate
                : colorMetric == ColorMetric::Cadence
                    ? RouteColorMode::Cadence
                    : colorMetric == ColorMetric::Speed
                        ? RouteColorMode::Speed
                        : RouteColorMode::Altitude;
    m_colorContext = colorContext;

    for (const auto& r : rideData.records)
        if (r.hasGps) { m_hasGps = true; break; }

    if (m_hasGps)
        fitToTrack();

    update();
}

void MapRenderer::setVisibleTimeRange(double startSeconds, double endSeconds)
{
    m_visibleStartSeconds = std::min(startSeconds, endSeconds);
    m_visibleEndSeconds = std::max(startSeconds, endSeconds);

    if (m_autoFitVisibleRange && !m_userMovedMap)
        fitToVisibleRange();

    update();
}

void MapRenderer::setCurrentTime(double seconds)
{
    m_currentTimeSeconds = seconds;
    update();
}

void MapRenderer::setRouteColorMode(RouteColorMode mode, const ColorContext& colorContext)
{
    m_routeColorMode = mode;
    m_colorContext = colorContext;
    update();
}

void MapRenderer::setMapStyle(MapStyle style)
{
    if (m_tileCache.mapStyle() == style)
        return;

    m_tileCache.setMapStyle(style);
    m_zoom = std::clamp(m_zoom, m_minZoom, std::min(18, m_tileCache.maxZoom()));
    update();
}

void MapRenderer::setAutoRouteContrast(bool enabled)
{
    if (m_autoRouteContrast == enabled)
        return;
    m_autoRouteContrast = enabled;
    update();
}

QColor MapRenderer::adjustRouteColorForStyle(const QColor& color) const
{
    if (!m_autoRouteContrast)
        return color;

    QColor adjusted = color;
    switch (m_tileCache.mapStyle())
    {
        case MapStyle::Dark:
        case MapStyle::Satellite:
            adjusted = adjusted.lighter(145);
            break;
        case MapStyle::Light:
        case MapStyle::Street:
        case MapStyle::Minimal:
            adjusted = adjusted.darker(120);
            break;
        case MapStyle::Terrain:
        case MapStyle::Topographic:
            adjusted = adjusted.lighter(120);
            break;
    }

    return adjusted;
}

void MapRenderer::fitToTrack()
{
    if (!m_hasGps) return;

    double minLat =  90.0;
    double maxLat = -90.0;
    double minLon = 180.0;
    double maxLon = -180.0;

    for (const auto& record : m_rideData.records)
    {
        if (!record.hasGps)
            continue;
        minLat = std::min(minLat, record.latitude);
        maxLat = std::max(maxLat, record.latitude);
        minLon = std::min(minLon, record.longitude);
        maxLon = std::max(maxLon, record.longitude);
    }

    m_userMovedMap = false;
    m_autoFitVisibleRange = true;

    fitToBounds(minLat, maxLat, minLon, maxLon, true);
    update();
}

void MapRenderer::fitToVisibleRange()
{
    if (!m_hasGps)
        return;

    double minLat =  90.0;
    double maxLat = -90.0;
    double minLon = 180.0;
    double maxLon = -180.0;
    int count = 0;

    for (const auto& record : m_rideData.records)
    {
        if (!record.hasGps)
            continue;
        if (m_visibleStartSeconds >= 0.0 && record.elapsedSeconds < m_visibleStartSeconds)
            continue;
        if (m_visibleEndSeconds >= 0.0 && record.elapsedSeconds > m_visibleEndSeconds)
            continue;

        minLat = std::min(minLat, record.latitude);
        maxLat = std::max(maxLat, record.latitude);
        minLon = std::min(minLon, record.longitude);
        maxLon = std::max(maxLon, record.longitude);
        ++count;
    }

    if (count < 1)
    {
        fitToTrack();
        return;
    }

    m_userMovedMap = false;
    m_autoFitVisibleRange = true;

    fitToBounds(minLat, maxLat, minLon, maxLon, false);
    update();
}

void MapRenderer::fitToBounds(double minLat, double maxLat, double minLon, double maxLon, bool updateMinZoom)
{
    if (!m_hasGps)
        return;

    const auto fit = MapFitMath::computeFitToBounds(minLat,
                                                     maxLat,
                                                     minLon,
                                                     maxLon,
                                                     width(),
                                                     height(),
                                                     std::min(18, m_tileCache.maxZoom()));

    m_minLat = fit.minLat;
    m_maxLat = fit.maxLat;
    m_minLon = fit.minLon;
    m_maxLon = fit.maxLon;
    m_centerLat = fit.centerLat;
    m_centerLon = fit.centerLon;
    m_zoom = fit.zoom;

    if (updateMinZoom)
        m_minZoom = m_zoom;   // track-level minimum zoom; selection fit must not tighten this floor
}

void MapRenderer::zoomIn()
{
    m_zoom = std::clamp(m_zoom + 1, m_minZoom, std::min(18, m_tileCache.maxZoom()));
    m_userMovedMap = true;
    m_autoFitVisibleRange = false;
    update();
}

void MapRenderer::zoomOut()
{
    m_zoom = std::clamp(m_zoom - 1, m_minZoom, std::min(18, m_tileCache.maxZoom()));
    m_userMovedMap = true;
    m_autoFitVisibleRange = false;
    update();
}

void MapRenderer::setHighlightedElapsedSeconds(double elapsedSeconds)
{
    m_highlightElapsedSeconds = elapsedSeconds;
    m_currentTimeSeconds = elapsedSeconds;
    update();
}

void MapRenderer::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(200, 200, 200));

    if (!m_hasGps)
    {
        p.drawText(rect(), Qt::AlignCenter, "No GPS data in this file");
        return;
    }

    const auto centreTile = latLonToTile(m_centerLat, m_centerLon, m_zoom);
    const int tileX0 = static_cast<int>(std::floor(
        centreTile.x() - static_cast<double>(width())  / (2.0 * kTilePx))) - 1;
    const int tileY0 = static_cast<int>(std::floor(
        centreTile.y() - static_cast<double>(height()) / (2.0 * kTilePx))) - 1;
    const int nX = static_cast<int>(std::ceil(
        static_cast<double>(width())  / kTilePx)) + 3;
    const int nY = static_cast<int>(std::ceil(
        static_cast<double>(height()) / kTilePx)) + 3;
    const int maxTile = (1 << m_zoom) - 1;

    for (int ty = 0; ty <= nY; ++ty)
    {
        for (int tx = 0; tx <= nX; ++tx)
        {
            const int tileX = tileX0 + tx;
            const int tileY = tileY0 + ty;
            if (tileX < 0 || tileX > maxTile || tileY < 0 || tileY > maxTile)
                continue;

            const QPointF topLeft = tileToScreen({ (double)tileX, (double)tileY });
            const QPixmap px = m_tileCache.tile(m_zoom, tileX, tileY);

            if (px.isNull())
                p.fillRect(QRectF(topLeft, QSizeF(kTilePx, kTilePx)), QColor(210,210,210));
            else
                p.drawPixmap(static_cast<int>(topLeft.x()),
                             static_cast<int>(topLeft.y()),
                             kTilePx, kTilePx, px);
        }
    }

    std::vector<const RideRecord*> gpsRecords;
    gpsRecords.reserve(m_rideData.records.size());
    for (const auto& record : m_rideData.records)
    {
        if (record.hasGps)
            gpsRecords.push_back(&record);
    }

    auto gpsValueForMode = [this](const RideRecord& record, const RideRecord* next, double& outValue, bool& outHasValue)
    {
        outHasValue = false;
        outValue = 0.0;
        switch (m_routeColorMode)
        {
            case RouteColorMode::None:
                return;
            case RouteColorMode::Power:
                if (record.hasPower) { outValue = record.power; outHasValue = true; }
                return;
            case RouteColorMode::HeartRate:
                if (record.hasHeartRate) { outValue = record.heartRate; outHasValue = true; }
                return;
            case RouteColorMode::Cadence:
                if (record.hasCadence) { outValue = record.cadence; outHasValue = true; }
                return;
            case RouteColorMode::Speed:
                if (record.hasSpeed) { outValue = record.speed; outHasValue = true; }
                return;
            case RouteColorMode::Altitude:
                if (record.hasAltitude) { outValue = record.altitude; outHasValue = true; }
                return;
            case RouteColorMode::Gradient:
                if (next && record.hasGps && next->hasGps)
                {
                    const double lat1 = record.latitude * kPi / 180.0;
                    const double lat2 = next->latitude * kPi / 180.0;
                    const double dLat = lat2 - lat1;
                    const double dLon = (next->longitude - record.longitude) * kPi / 180.0;
                    const double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0)
                                   + std::cos(lat1) * std::cos(lat2)
                                   * std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
                    const double distance = 2.0 * 6371000.0 * std::atan2(std::sqrt(a), std::sqrt(std::max(0.0, 1.0 - a)));
                    const double elevation = next->altitude - record.altitude;
                    if (distance > 0.1)
                    {
                        outValue = (elevation / distance) * 100.0;
                        outHasValue = true;
                    }
                }
                return;
        }
    };

    auto segmentVisible = [this](double startSec, double endSec)
    {
        if (m_visibleStartSeconds < 0.0 || m_visibleEndSeconds < 0.0)
            return true;
        const double lo = std::min(m_visibleStartSeconds, m_visibleEndSeconds);
        const double hi = std::max(m_visibleStartSeconds, m_visibleEndSeconds);
        return endSec >= lo && startSec <= hi;
    };

    auto drawSegment = [&](const QPointF& a, const QPointF& b, const QColor& color, qreal width)
    {
        p.setPen(QPen(color, width));
        p.drawLine(a, b);
    };

    auto gpsPointAtTime = [&gpsRecords](double elapsedSeconds) -> const RideRecord*
    {
        if (gpsRecords.empty())
            return nullptr;

        const RideRecord* nearest = nullptr;
        double bestDist = std::numeric_limits<double>::max();
        for (const RideRecord* record : gpsRecords)
        {
            const double dist = std::abs(record->elapsedSeconds - elapsedSeconds);
            if (dist < bestDist)
            {
                bestDist = dist;
                nearest = record;
            }
        }
        return nearest;
    };

    const bool hasVisibleRange = m_visibleStartSeconds >= 0.0 && m_visibleEndSeconds >= 0.0;
    const QColor mutedGrey = [&]()
    {
        QColor c("#d1d5db");
        c.setAlpha(80);
        return c;
    }();

    if (m_routeColorMode == RouteColorMode::None)
    {
        QPainterPath path;
        bool first = true;
        for (const RideRecord* record : gpsRecords)
        {
            const auto sp = tileToScreen(latLonToTile(record->latitude, record->longitude, m_zoom));
            if (first) { path.moveTo(sp); first = false; }
            else       { path.lineTo(sp); }
        }
        p.setPen(QPen(mutedGrey, 1.5));
        p.drawPath(path);

        QPainterPath visiblePath;
        first = true;
        for (size_t index = 1; index < gpsRecords.size(); ++index)
        {
            const RideRecord& prev = *gpsRecords[index - 1];
            const RideRecord& curr = *gpsRecords[index];
            if (!segmentVisible(prev.elapsedSeconds, curr.elapsedSeconds))
                continue;

            const auto sp = tileToScreen(latLonToTile(prev.latitude, prev.longitude, m_zoom));
            const auto ep = tileToScreen(latLonToTile(curr.latitude, curr.longitude, m_zoom));
            if (first) { visiblePath.moveTo(sp); first = false; }
            visiblePath.lineTo(ep);
        }
        p.setPen(QPen(adjustRouteColorForStyle(QColor(37, 99, 235)), 4.5));
        p.drawPath(visiblePath);
    }
    else
    {
        for (size_t index = 1; index < gpsRecords.size(); ++index)
        {
            const RideRecord& prev = *gpsRecords[index - 1];
            const RideRecord& curr = *gpsRecords[index];
            const QPointF prevPoint = tileToScreen(latLonToTile(prev.latitude, prev.longitude, m_zoom));
            const QPointF currPoint = tileToScreen(latLonToTile(curr.latitude, curr.longitude, m_zoom));

            double value = 0.0;
            bool hasValue = false;
            gpsValueForMode(prev, &curr, value, hasValue);
            if (!hasValue)
                gpsValueForMode(curr, nullptr, value, hasValue);

            const QColor color = hasValue
                ? (m_routeColorMode == RouteColorMode::Gradient
                    ? adjustRouteColorForStyle(gradientColorForSlope(value))
                    : adjustRouteColorForStyle(ColorProvider::colorForMetric(routeModeToMetric(m_routeColorMode), value, m_colorContext)))
                : QColor("#cbd5e1");

            if (!segmentVisible(prev.elapsedSeconds, curr.elapsedSeconds))
            {
                drawSegment(prevPoint, currPoint, mutedGrey, 1.5);
            }
            else
            {
                drawSegment(prevPoint, currPoint, mutedGrey, 1.5);
                drawSegment(prevPoint, currPoint, color, 4.5);
            }
        }
    }

    if (hasVisibleRange)
    {
        const RideRecord* startRecord = gpsPointAtTime(m_visibleStartSeconds);
        const RideRecord* endRecord = gpsPointAtTime(m_visibleEndSeconds);

        auto drawMarker = [&](const RideRecord* record, const QColor& color)
        {
            if (!record)
                return;
            const QPointF point = tileToScreen(latLonToTile(record->latitude, record->longitude, m_zoom));
            p.setPen(QPen(QColor("#ffffff"), 2.0));
            p.setBrush(color);
            p.drawEllipse(point, 6.5, 6.5);
        };

        drawMarker(startRecord, QColor("#22c55e"));
        drawMarker(endRecord, QColor("#ef4444"));
    }

    auto drawDot = [&](double lat, double lon, QColor c)
    {
        const auto sp = tileToScreen(latLonToTile(lat, lon, m_zoom));
        p.setPen(QPen(Qt::white, 2.0));
        p.setBrush(c);
        p.drawEllipse(sp, 6.0, 6.0);
    };
    for (const auto& r : m_rideData.records)
        if (r.hasGps) { drawDot(r.latitude, r.longitude, QColor(50,180,50)); break; }
    for (auto it = m_rideData.records.rbegin(); it != m_rideData.records.rend(); ++it)
        if (it->hasGps) { drawDot(it->latitude, it->longitude, QColor(220,50,50)); break; }

    if (m_currentTimeSeconds >= 0.0)
    {
        const RideRecord* nearest = nullptr;
        double bestDist = std::numeric_limits<double>::max();
        for (const RideRecord& r : m_rideData.records)
        {
            if (!r.hasGps)
                continue;
            const double d = std::abs(r.elapsedSeconds - m_currentTimeSeconds);
            if (d < bestDist)
            {
                bestDist = d;
                nearest = &r;
            }
        }

        if (nearest)
        {
            const QPointF hp = tileToScreen(latLonToTile(nearest->latitude, nearest->longitude, m_zoom));
            p.setPen(QPen(QColor("#f8fafc"), 2.0));
            p.setBrush(QColor("#111827"));
            p.drawEllipse(hp, 7.0, 7.0);
            p.setPen(QPen(QColor("#ef4444"), 2.0));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(hp, 11.0, 11.0);
        }
    }

    p.setPen(QColor(60, 60, 60));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect().adjusted(0,0,-4,-4),
               Qt::AlignBottom | Qt::AlignRight,
               m_tileCache.attribution());
}

void MapRenderer::wheelEvent(QWheelEvent* event)
{
    const int delta = event->angleDelta().y() > 0 ? 1 : -1;
    const int oldZoom = m_zoom;
    const int newZoom = std::clamp(m_zoom + delta, m_minZoom, std::min(18, m_tileCache.maxZoom()));
    if (newZoom == oldZoom)
    {
        event->accept();
        return;
    }

    const QPointF cursor = event->position();
    const QPointF oldCenterTile = latLonToTile(m_centerLat, m_centerLon, oldZoom);
    const QPointF screenOffset(
        (cursor.x() - width() / 2.0) / kTilePx,
        (cursor.y() - height() / 2.0) / kTilePx
    );

    // World position under the cursor before zoom.
    const QPointF anchorTileOld = oldCenterTile + screenOffset;
    const double nOld = std::pow(2.0, oldZoom);
    const double anchorLon = anchorTileOld.x() / nOld * 360.0 - 180.0;
    const double anchorMercN = kPi * (1.0 - 2.0 * anchorTileOld.y() / nOld);
    const double anchorLat = std::atan(std::sinh(anchorMercN)) * 180.0 / kPi;

    // Recompute center at new zoom so the same world point stays under cursor.
    const QPointF anchorTileNew = latLonToTile(anchorLat, anchorLon, newZoom);
    const QPointF newCenterTile = anchorTileNew - screenOffset;
    const double nNew = std::pow(2.0, newZoom);
    m_centerLon = newCenterTile.x() / nNew * 360.0 - 180.0;
    const double centerMercN = kPi * (1.0 - 2.0 * newCenterTile.y() / nNew);
    m_centerLat = std::atan(std::sinh(centerMercN)) * 180.0 / kPi;
    m_centerLat = std::clamp(m_centerLat, -85.051129, 85.051129);
    m_zoom = newZoom;
    m_userMovedMap = true;
    m_autoFitVisibleRange = false;

    update();
    event->accept();
}

void MapRenderer::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!m_hasGps || event->button() != Qt::LeftButton)
    {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    const QPointF centreTile = latLonToTile(m_centerLat, m_centerLon, m_zoom);
    const double clickedTileX = centreTile.x() + (event->position().x() - width() / 2.0) / kTilePx;
    const double clickedTileY = centreTile.y() + (event->position().y() - height() / 2.0) / kTilePx;

    const double n = std::pow(2.0, m_zoom);
    m_centerLon = clickedTileX / n * 360.0 - 180.0;
    const double mercN = kPi * (1.0 - 2.0 * clickedTileY / n);
    m_centerLat = std::atan(std::sinh(mercN)) * 180.0 / kPi;
    m_centerLat = std::clamp(m_centerLat, -85.051129, 85.051129);

    m_zoom = std::clamp(m_zoom + 1, m_minZoom, std::min(18, m_tileCache.maxZoom()));
    m_panning = false;
    m_userMovedMap = true;
    m_autoFitVisibleRange = false;
    update();
    event->accept();
}

void MapRenderer::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_panning  = true;
        m_panStart = event->position();
    }
}

void MapRenderer::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_panning) return;
    const QPointF delta = event->position() - m_panStart;
    m_panStart = event->position();

    const auto prevTile = latLonToTile(m_centerLat, m_centerLon, m_zoom);
    const double newTileX = prevTile.x() - delta.x() / kTilePx;
    const double newTileY = prevTile.y() - delta.y() / kTilePx;

    const double n = std::pow(2.0, m_zoom);
    m_centerLon = newTileX / n * 360.0 - 180.0;
    const double mercN = kPi * (1.0 - 2.0 * newTileY / n);
    m_centerLat = std::atan(std::sinh(mercN)) * 180.0 / kPi;
    m_centerLat = std::clamp(m_centerLat, -85.051129, 85.051129);
    m_userMovedMap = true;
    m_autoFitVisibleRange = false;
    update();
}

void MapRenderer::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        m_panning = false;
}
