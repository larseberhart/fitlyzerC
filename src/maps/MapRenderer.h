#pragma once

#include <QPointF>
#include <QRectF>
#include <QWidget>

#include "core/zones/ZoneDefinition.h"
#include "fit/RideData.h"
#include "maps/TileCache.h"

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

class MapRenderer : public QWidget
{
    Q_OBJECT
public:
    explicit MapRenderer(QWidget* parent = nullptr);

    void setRideData(const RideData& rideData,
                     ColorMetric colorMetric = ColorMetric::None,
                     const ColorContext& colorContext = {});
    void setVisibleTimeRange(double startSeconds, double endSeconds);
    void setCurrentTime(double seconds);
    void setRouteColorMode(RouteColorMode mode, const ColorContext& colorContext = {});
    void setMapStyle(MapStyle style);
    MapStyle mapStyle() const { return m_tileCache.mapStyle(); }
    void setAutoRouteContrast(bool enabled);
    bool autoRouteContrast() const { return m_autoRouteContrast; }

signals:
    void segmentSelectionChanged(double startSeconds, double endSeconds);
    void segmentSelectionFinished(double startSeconds, double endSeconds);

public slots:
    void fitToTrack();
    void fitToVisibleRange();
    void zoomIn();
    void zoomOut();
    void setHighlightedElapsedSeconds(double elapsedSeconds);

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
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
    const RideRecord* gpsRecordAtTime(double elapsedSeconds) const;

    TileCache m_tileCache;
    RideData  m_rideData;
    // Pre-filtered, time-ordered GPS-only records. Rebuilt once in setRideData().
    std::vector<const RideRecord*> m_gpsRecords;
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
};
