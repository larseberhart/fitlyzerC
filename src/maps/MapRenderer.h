// SPDX-License-Identifier: GPL-3

/**
 * @file MapRenderer.h
 * @brief Map rendering support for MapRenderer.
 *
 * Implements map math, tile caching, and rendering logic used for geographic visualization of activities.
 *
 * Responsibilities:
 * - Provide map computation, tile handling, or rendering functionality
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QPointF>
#include <QRectF>
#include <QTimer>
#include <QWidget>

#include "core/zones/ZoneDefinition.h"
#include "fit/RideData.h"
#include "maps/TileCache.h"

/**
 * @brief Route color visualization modes.
 */
enum class RouteColorMode
{
    None,
    Power,
    HeartRate,
    Cadence,
    Speed,
    Altitude,
    Gradient
};

/**
 * @brief Interactive map widget for displaying and editing activity routes.
 *
 * Renders map tiles with activity GPS trace, supports color-coding by metrics,
 * time-range selection, and interacts with the main UI.
 */
class MapRenderer : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief Constructs the map renderer.
     * @param parent Parent widget.
     */
    explicit MapRenderer(QWidget* parent = nullptr);

    /**
 * @brief Sets activity data to display on map.
 * @param rideData Activity data.
 * @param colorMetric Metric to use for route coloring.
 * @param colorContext Athlete context for coloring.
 */
void setRideData(const RideData& rideData,
                 ColorMetric colorMetric = ColorMetric::None,
                 const ColorContext& colorContext = {});

/**
 * @brief Sets visible time range for segmented display.
 * @param startSeconds Start time in seconds from activity start.
 * @param endSeconds End time in seconds from activity start.
 */
void setVisibleTimeRange(double startSeconds, double endSeconds);

/**
 * @brief Sets current time marker position.
 * @param seconds Current time in seconds from activity start.
 */
void setCurrentTime(double seconds);

/**
 * @brief Sets route color visualization mode.
 * @param mode Color mode (Power, HeartRate, etc.).
 * @param colorContext Athlete context for coloring.
 */
void setRouteColorMode(RouteColorMode mode, const ColorContext& colorContext = {});

/**
 * @brief Sets map tile style.
 * @param style Map style (Light, Dark, Satellite, etc.).
 */
void setMapStyle(MapStyle style);

/**
 * @brief Gets current map style.
 * @return Current style.
 */
MapStyle mapStyle() const { return m_tileCache.mapStyle(); }

/**
 * @brief Sets automatic route contrast adjustment.
 * @param enabled True to enable auto contrast.
 */
void setAutoRouteContrast(bool enabled);

/**
 * @brief Gets auto route contrast setting.
 * @return True if auto contrast enabled.
 */
bool autoRouteContrast() const { return m_autoRouteContrast; }

/**
 * @brief Sets tile cache size limit.
 * @param maxTiles Maximum tiles to cache in memory.
 */
void setTileCacheSize(int maxTiles);

signals:
    /// \signal Emitted when segment selection changes (during dragging).
    /// \param startSeconds Current segment start time in seconds.
    /// \param endSeconds Current segment end time in seconds.
    void segmentSelectionChanged(double startSeconds, double endSeconds);

    /// \signal Emitted when segment selection completes.
    /// \param startSeconds Final segment start time in seconds.
    /// \param endSeconds Final segment end time in seconds.
    void segmentSelectionFinished(double startSeconds, double endSeconds);

public slots:
    /// \slot Fits map to entire track bounds.
    void fitToTrack();

    /// \slot Fits map to visible time range bounds.
    void fitToVisibleRange();

    /// \slot Zooms map in.
    void zoomIn();

    /// \slot Zooms map out.
    void zoomOut();

    /// \slot Sets highlighted current time marker.
    /// \param elapsedSeconds Elapsed time in seconds from activity start.
    void setHighlightedElapsedSeconds(double elapsedSeconds);

protected:
    /// @brief Renders map tiles and route on update.
    void paintEvent(QPaintEvent* event) override;

    /// @brief Handles scroll wheel for map zoom.
    void wheelEvent(QWheelEvent* event) override;

    /// @brief Handles double-click for fit-to-track.
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    /// @brief Handles mouse press for segment selection start.
    void mousePressEvent(QMouseEvent* event) override;

    /// @brief Handles mouse movement for segment selection.
    void mouseMoveEvent(QMouseEvent* event) override;

    /// @brief Handles mouse release for segment selection end.
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    enum class DragHandle
    {
        None,
        Start,
        End
    };

    static QPointF latLonToTile(double lat, double lon, int zoom);
    void fitToBounds(double minLat, double maxLat, double minLon, double maxLon, bool updateMinZoom);
    QPointF        tileToScreen(const QPointF& tilePt) const;
    QColor adjustRouteColorForStyle(const QColor& color) const;
    const RideRecord* nearestGpsRecord(const QPointF& screenPos) const;
    void rebuildGpsCache();
    void rebuildSegmentColors();
    const RideRecord* gpsRecordAtTime(double elapsedSeconds) const;

    TileCache m_tileCache;
    RideData  m_rideData;
    // WARNING: m_gpsRecords contains raw pointers into m_rideData.records.
    // Any mutation of m_rideData.records requires calling rebuildGpsCache().
    // Pre-filtered, time-ordered GPS-only records. Rebuilt once in setRideData().
    std::vector<const RideRecord*> m_gpsRecords;
    // Per-segment base colors (before map-style adjustment). One per GPS segment.
    // Invalid QColor means no data for that segment.
    std::vector<QColor> m_segmentColors;
    const RideRecord* m_firstGpsRecord = nullptr;
    const RideRecord* m_lastGpsRecord  = nullptr;
    bool      m_hasGps = false;
    RouteColorMode m_routeColorMode = RouteColorMode::None;
    ColorContext m_colorContext;

    double m_minLat =  90.0, m_maxLat = -90.0;
    double m_minLon = 180.0, m_maxLon = -180.0;

    double  m_centerLat = 51.5;
    double  m_centerLon = 0.0;
    int     m_zoom      = 13;
    int     m_minZoom   = 1;   // set by fitToTrack(); prevents zooming out past track extent

    QPointF m_panStart;
    bool    m_panning = false;
    DragHandle m_dragHandle = DragHandle::None;
    QRectF  m_startMarkerRect;
    QRectF  m_endMarkerRect;
    bool    m_draggingSelection = false;
    bool    m_userMovedMap = false;
    bool    m_autoFitVisibleRange = true;
    bool    m_autoRouteContrast = true;
    double  m_highlightElapsedSeconds = -1.0;
    double  m_visibleStartSeconds = -1.0;
    double  m_visibleEndSeconds = -1.0;
    double  m_currentTimeSeconds = -1.0;
    QTimer  m_tileRepaintTimer;
};