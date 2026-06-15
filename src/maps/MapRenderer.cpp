// SPDX-License-Identifier: GPL-3

/**
 * @file MapRenderer.cpp
 * @brief Map rendering support for MapRenderer.
 *
 * Implements map math, tile caching, and rendering logic used for geographic visualization of activities.
 *
 * Responsibilities:
 * - Provide map computation, tile handling, or rendering functionality
 *
 * @author Lars EBERHART
 */

#include "MapRenderer.h"

#include <QLineF>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>

#include "core/settings/AppSettings.h"
#include "core/zones/ColorProvider.h"
#include "maps/MapFitMath.h"

#include <algorithm>
#include <cmath>
#include <limits>

/**
 * @brief Computes color for gradient visualization.
 * @param percent Slope gradient percentage.
 * @return Color from brown (downhill) to green (steep uphill).
 */
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

/**
 * @brief Converts route color mode to color metric for data lookup.
 * @param mode Route visualization mode.
 * @return Corresponding color metric (None if gradient mode).
 */
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

/// @brief Mathematical constant pi.
static constexpr double kPi     = 3.14159265358979323846;

/// @brief Tile size in pixels (Web Mercator standard).
static constexpr int    kTilePx = 256;

/// @brief Hit target size for climb/interval markers in pixels.
static constexpr qreal  kMarkerHitSizePx = 24.0;

/// @brief Display radius for climb/interval markers in pixels.
static constexpr qreal  kMarkerRadiusPx = 6.5;

/// @brief Maximum pixel distance for snapping to nearest record.
static constexpr qreal  kNearestRecordMaxDistancePx = 40.0;

/// @brief Margin from screen edge for panning triggers in pixels.
static constexpr int    kEdgePanMargin = 60;

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

    // Apply persisted tile cache size.
    m_tileCache.setMemoryCacheSize(AppSettings::instance().tileCacheSize());

    // Coalesce tile-download repaints: many tiles arriving at once
    // produce one repaint 50 ms after the last arrival, not one per tile.
    m_tileRepaintTimer.setSingleShot(true);
    m_tileRepaintTimer.setInterval(50);
    connect(&m_tileRepaintTimer, &QTimer::timeout, this, [this] { update(); });

    connect(&m_tileCache, &TileCache::tileLoaded,
            this, [this](int, int, int) { m_tileRepaintTimer.start(); });
}

// Precompute per-segment base colors (excluding map-style adjustment which changes
// per-repaint). Called once on setRideData() and whenever the color mode changes.
void MapRenderer::rebuildSegmentColors()
{
    if (m_gpsRecords.size() < 2 || m_routeColorMode == RouteColorMode::None)
    {
        m_segmentColors.clear();
        return;
    }

    m_segmentColors.resize(m_gpsRecords.size() - 1);

    for (size_t i = 0; i + 1 < m_gpsRecords.size(); ++i)
    {
        const RideRecord& cur  = *m_gpsRecords[i];
        const RideRecord& next = *m_gpsRecords[i + 1];

        double value = 0.0;
        bool hasValue = false;

        switch (m_routeColorMode)
        {
            case RouteColorMode::None: break;
            case RouteColorMode::Power:
                if      (cur.hasPower)  { value = cur.power;  hasValue = true; }
                else if (next.hasPower) { value = next.power; hasValue = true; }
                break;
            case RouteColorMode::HeartRate:
                if      (cur.hasHeartRate)  { value = cur.heartRate;  hasValue = true; }
                else if (next.hasHeartRate) { value = next.heartRate; hasValue = true; }
                break;
            case RouteColorMode::Cadence:
                if      (cur.hasCadence)  { value = cur.cadence;  hasValue = true; }
                else if (next.hasCadence) { value = next.cadence; hasValue = true; }
                break;
            case RouteColorMode::Speed:
                if      (cur.hasSpeed)  { value = cur.speed;  hasValue = true; }
                else if (next.hasSpeed) { value = next.speed; hasValue = true; }
                break;
            case RouteColorMode::Altitude:
                if      (cur.hasAltitude)  { value = cur.altitude;  hasValue = true; }
                else if (next.hasAltitude) { value = next.altitude; hasValue = true; }
                break;
            case RouteColorMode::Gradient:
                if (cur.hasGps && next.hasGps && cur.hasAltitude && next.hasAltitude)
                {
                    const double lat1 = cur.latitude  * kPi / 180.0;
                    const double lat2 = next.latitude * kPi / 180.0;
                    const double dLat = lat2 - lat1;
                    const double dLon = (next.longitude - cur.longitude) * kPi / 180.0;
                    const double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0)
                                   + std::cos(lat1) * std::cos(lat2)
                                   * std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
                    const double dist = 2.0 * 6371000.0
                        * std::atan2(std::sqrt(a), std::sqrt(std::max(0.0, 1.0 - a)));
                    const double elev = next.altitude - cur.altitude;
                    if (dist > 0.1) { value = (elev / dist) * 100.0; hasValue = true; }
                }
                break;
        }

        m_segmentColors[i] = hasValue
            ? (m_routeColorMode == RouteColorMode::Gradient
                ? gradientColorForSlope(value)
                : ColorProvider::colorForMetric(
                    routeModeToMetric(m_routeColorMode), value, m_colorContext))
            : QColor(); // invalid = no data
    }
}

void MapRenderer::rebuildGpsCache()
{
    m_gpsRecords.clear();
    m_gpsRecords.reserve(m_rideData.records.size());
    m_firstGpsRecord = nullptr;
    m_lastGpsRecord  = nullptr;

    for (const auto& r : m_rideData.records)
    {
        if (!r.hasGps)
            continue;
        m_gpsRecords.push_back(&r);
        if (!m_firstGpsRecord)
            m_firstGpsRecord = &r;
        m_lastGpsRecord = &r;
    }
    // Records are already in elapsedSeconds order from FIT parsing.
}

// Nearest GPS record to a given elapsed time: O(log N).
const RideRecord* MapRenderer::gpsRecordAtTime(double elapsedSeconds) const
{
    if (m_gpsRecords.empty())
        return nullptr;

    auto it = std::lower_bound(
        m_gpsRecords.begin(), m_gpsRecords.end(), elapsedSeconds,
        [](const RideRecord* r, double t) { return r->elapsedSeconds < t; });

    if (it == m_gpsRecords.end())
        return m_gpsRecords.back();
    if (it == m_gpsRecords.begin())
        return m_gpsRecords.front();

    auto prev = std::prev(it);
    return (std::abs((*prev)->elapsedSeconds - elapsedSeconds) <=
            std::abs((*it)->elapsedSeconds  - elapsedSeconds))
        ? *prev : *it;
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
    m_dragHandle = DragHandle::None;
    m_startMarkerRect = QRectF();
    m_endMarkerRect = QRectF();
    m_draggingSelection = false;
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

    rebuildGpsCache();
    m_hasGps = !m_gpsRecords.empty();
    rebuildSegmentColors();

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
    rebuildSegmentColors();
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

void MapRenderer::setTileCacheSize(int maxTiles)
{
    m_tileCache.setMemoryCacheSize(maxTiles);
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

const RideRecord* MapRenderer::nearestGpsRecord(const QPointF& screenPos) const
{
    if (m_gpsRecords.empty())
        return nullptr;

    const RideRecord* nearest = nullptr;
    double bestDistance = std::numeric_limits<double>::max();

    for (const RideRecord* record : m_gpsRecords)
    {
        const QPointF point = tileToScreen(latLonToTile(record->latitude, record->longitude, m_zoom));
        const double distance = QLineF(screenPos, point).length();
        if (distance < bestDistance)
        {
            bestDistance = distance;
            nearest = record;
        }
    }

    if (bestDistance > kNearestRecordMaxDistancePx)
        return nullptr;

    return nearest;
}

void MapRenderer::fitToTrack()
{
    if (!m_hasGps) return;

    double minLat =  90.0;
    double maxLat = -90.0;
    double minLon = 180.0;
    double maxLon = -180.0;

    for (const RideRecord* record : m_gpsRecords)
    {
        minLat = std::min(minLat, record->latitude);
        maxLat = std::max(maxLat, record->latitude);
        minLon = std::min(minLon, record->longitude);
        maxLon = std::max(maxLon, record->longitude);
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

    // Use lower_bound to skip records before the visible range start.
    auto rangeBegin = m_gpsRecords.begin();
    if (m_visibleStartSeconds >= 0.0)
    {
        rangeBegin = std::lower_bound(
            m_gpsRecords.begin(), m_gpsRecords.end(), m_visibleStartSeconds,
            [](const RideRecord* r, double t) { return r->elapsedSeconds < t; });
    }

    for (auto it = rangeBegin; it != m_gpsRecords.end(); ++it)
    {
        const RideRecord* record = *it;
        if (m_visibleEndSeconds >= 0.0 && record->elapsedSeconds > m_visibleEndSeconds)
            break;

        minLat = std::min(minLat, record->latitude);
        maxLat = std::max(maxLat, record->latitude);
        minLon = std::min(minLon, record->longitude);
        maxLon = std::max(maxLon, record->longitude);
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

    // Use precomputed GPS record list — no per-frame allocation.
    const auto& gpsRecords = m_gpsRecords;

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

    // O(log N) time lookup using precomputed sorted GPS records.
    auto gpsPointAtTime = [this](double elapsedSeconds) -> const RideRecord*
    {
        return gpsRecordAtTime(elapsedSeconds);
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

            // Look up precomputed base color; apply style adjustment per-paint.
            const QColor& baseColor = m_segmentColors[index - 1];
            const QColor color = baseColor.isValid()
                ? adjustRouteColorForStyle(baseColor)
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

    m_startMarkerRect = QRectF();
    m_endMarkerRect = QRectF();

    if (hasVisibleRange)
    {
        const RideRecord* startRecord = gpsPointAtTime(m_visibleStartSeconds);
        const RideRecord* endRecord = gpsPointAtTime(m_visibleEndSeconds);

        auto drawMarker = [&](const RideRecord* record, const QColor& color, QRectF* markerRect)
        {
            if (!record)
                return;
            const QPointF point = tileToScreen(latLonToTile(record->latitude, record->longitude, m_zoom));
            if (markerRect)
            {
                *markerRect = QRectF(point.x() - kMarkerHitSizePx / 2.0,
                                     point.y() - kMarkerHitSizePx / 2.0,
                                     kMarkerHitSizePx,
                                     kMarkerHitSizePx);
            }
            p.setPen(QPen(QColor("#ffffff"), 2.0));
            p.setBrush(color);
            p.drawEllipse(point, kMarkerRadiusPx, kMarkerRadiusPx);
        };

        drawMarker(startRecord, QColor("#22c55e"), &m_startMarkerRect);
        drawMarker(endRecord, QColor("#ef4444"), &m_endMarkerRect);
    }

    auto drawDot = [&](double lat, double lon, QColor c)
    {
        const auto sp = tileToScreen(latLonToTile(lat, lon, m_zoom));
        p.setPen(QPen(Qt::white, 2.0));
        p.setBrush(c);
        p.drawEllipse(sp, 6.0, 6.0);
    };
    if (m_firstGpsRecord)
        drawDot(m_firstGpsRecord->latitude, m_firstGpsRecord->longitude, QColor(50,180,50));
    if (m_lastGpsRecord)
        drawDot(m_lastGpsRecord->latitude, m_lastGpsRecord->longitude, QColor(220,50,50));

    if (m_currentTimeSeconds >= 0.0)
    {
        const RideRecord* nearest = gpsRecordAtTime(m_currentTimeSeconds);

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
        const QPointF pos = event->position();
        if (m_startMarkerRect.contains(pos))
        {
            m_dragHandle = DragHandle::Start;
            m_draggingSelection = true;
            m_panning = false;
            event->accept();
            return;
        }

        if (m_endMarkerRect.contains(pos))
        {
            m_dragHandle = DragHandle::End;
            m_draggingSelection = true;
            m_panning = false;
            event->accept();
            return;
        }

        m_panning  = true;
        m_panStart = event->position();
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void MapRenderer::mouseMoveEvent(QMouseEvent* event)
{
    if (m_draggingSelection)
    {
        const QPointF pos = event->position();

        double panX = 0.0;
        double panY = 0.0;
        if (pos.x() < kEdgePanMargin)
            panX = -0.05;
        else if (pos.x() > width() - kEdgePanMargin)
            panX = 0.05;

        if (pos.y() < kEdgePanMargin)
            panY = -0.05;
        else if (pos.y() > height() - kEdgePanMargin)
            panY = 0.05;

        if (panX != 0.0 || panY != 0.0)
        {
            const QPointF prevTile = latLonToTile(m_centerLat, m_centerLon, m_zoom);
            const QPointF newTile = prevTile + QPointF(panX, panY);
            const double n = std::pow(2.0, m_zoom);
            m_centerLon = newTile.x() / n * 360.0 - 180.0;
            const double mercN = kPi * (1.0 - 2.0 * newTile.y() / n);
            m_centerLat = std::atan(std::sinh(mercN)) * 180.0 / kPi;
            m_centerLat = std::clamp(m_centerLat, -85.051129, 85.051129);
            m_userMovedMap = true;
            m_autoFitVisibleRange = false;
        }

        const RideRecord* record = nearestGpsRecord(pos);
        if (!record)
        {
            update();
            event->accept();
            return;
        }

        if (m_dragHandle == DragHandle::Start)
            m_visibleStartSeconds = std::min(record->elapsedSeconds, m_visibleEndSeconds);
        else if (m_dragHandle == DragHandle::End)
            m_visibleEndSeconds = std::max(record->elapsedSeconds, m_visibleStartSeconds);

        emit segmentSelectionChanged(m_visibleStartSeconds, m_visibleEndSeconds);
        update();
        event->accept();
        return;
    }

    if (!m_panning)
    {
        QWidget::mouseMoveEvent(event);
        return;
    }

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
    event->accept();
}

void MapRenderer::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (m_draggingSelection)
        {
            emit segmentSelectionFinished(m_visibleStartSeconds, m_visibleEndSeconds);
            m_draggingSelection = false;
            m_dragHandle = DragHandle::None;
            event->accept();
            return;
        }

        m_panning = false;
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}