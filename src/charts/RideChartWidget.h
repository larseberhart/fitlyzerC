#pragma once

#include <QtCharts/QChartView>

#include <limits>
#include <unordered_map>
#include <vector>

#include "analysis/IntervalDetector.h"
#include "analysis/ClimbMetrics.h"
#include "charts/Metric.h"
#include "core/zones/ZoneDefinition.h"
#include "fit/RideData.h"

class QLineSeries;
class QAreaSeries;
class QValueAxis;
class QCategoryAxis;

class RideChartWidget : public QChartView
{
    Q_OBJECT

public:
    explicit RideChartWidget(Metric metric, QWidget* parent = nullptr);

    void setRideData(const RideData& rideData,
                     ColorMetric colorMetric = ColorMetric::None,
                     const ColorContext& colorContext = {});
    void setPowerSmoothingSeconds(int seconds);
    void setAutoSmoothingEnabled(bool enabled);
    void setIntervals(const std::vector<Interval>& intervals);
    void setClimbs(const std::vector<Climb>& climbs);
    void setMetricOverlay(ColorMetric metric, bool enabled);
    void setSelectedClimbIndex(int index);
    void setClimbEditingEnabled(bool enabled);
    void clearChart();

    bool hasData() const { return m_hasData; }
    double visibleRangeStartMinutes() const;
    double visibleRangeEndMinutes() const;

public slots:
    void setXRange(double min, double max);
    void setCrosshair(double xMinutes);
    void resetZoom();
    void fitToData(); // alias for resetZoom — resets to full ride extent

signals:
    void xRangeChanged(double min, double max);
    void visibleRangeChanged(double startMinutes, double endMinutes);
    void crosshairMoved(double xMinutes);
    void intervalSelectionRequested(double startSeconds, double endSeconds);
    void climbBoundaryEdited(
        double oldStartSeconds,
        double oldEndSeconds,
        double newStartSeconds,
        double newEndSeconds);
    void newClimbRequested(double startSeconds, double endSeconds);
    void climbClicked(int climbIndex);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void rebuildChart(bool preserveXRange = true);
    std::vector<double> computeSmoothedSeries(Metric metric, int seconds) const;
    std::vector<double> computeSmoothedPowerSeries(int seconds) const;
    std::vector<int> buildSampleIndices(const std::vector<double>& values, int targetPointBudget) const;
    void clearBackgroundSeries();
    void addZoneBackgroundBands(double minY, double maxY, double maxXMinutes);
    void addMetricOverlayBands(double minY, double maxY);
    void addIntervalBackgroundBands(double minY, double maxY);
    void addClimbBackgroundBands(double minY, double maxY);
    void updateXTicks(double minVal, double maxVal);
    void updateYAxis();   // recalculates nice range + tick count from full data
    void updateYAxisForVisibleRange(double xMinMin, double xMaxMin); // rescale to visible window

    Metric          m_metric;
    QLineSeries*    m_series        = nullptr;
    std::vector<QLineSeries*> m_rawSeries;
    std::vector<QLineSeries*> m_colorSeries;
    std::vector<QLineSeries*> m_overlaySeries;
    std::vector<QLineSeries*> m_referenceSeries;
    std::vector<QAreaSeries*> m_backgroundSeries;
    QValueAxis*     m_axisX         = nullptr;
    QCategoryAxis*  m_axisXDisplay  = nullptr;
    QValueAxis*     m_axisY         = nullptr;
    QValueAxis*     m_axisYOverlay  = nullptr;
    RideData        m_rideData;
    std::vector<Interval> m_intervals;
    std::vector<Climb> m_climbs;
    bool            m_metricOverlayEnabled = false;
    ColorMetric     m_metricOverlayMetric = ColorMetric::None;
    ColorMetric     m_colorMetric   = ColorMetric::None;
    ColorContext    m_colorContext;

    // Cache for smoothed metric series: keyed by (metric * 1000 + smoothing_seconds)
    // Avoids recomputation when chart is redrawn or multiple overlays use same smoothing
    mutable std::unordered_map<int, std::vector<double>> m_smoothedSeriesCache;
    std::vector<QPointF> m_tooltipPoints;
    std::vector<double>  m_tooltipRawValues;
    std::vector<double>  m_tooltipDisplayValues;
    int             m_requestedPowerSmoothingSeconds = 0;
    int             m_effectivePowerSmoothingSeconds = 0;
    bool            m_autoSmoothing = false;
    bool            m_isRebuilding  = false;
    bool            m_syncing       = false;
    bool            m_hasData       = false;
    double          m_origMaxX      = 1.0;
    double          m_crosshairX    = -1.0;
    double          m_crosshairY    = std::numeric_limits<double>::quiet_NaN();
    bool            m_intervalSelecting = false;
    double          m_intervalStartMin = -1.0;
    double          m_intervalEndMin = -1.0;
    bool            m_climbEditingEnabled = false;
    int             m_selectedClimbIndex = -1;
    enum class ClimbHandleDrag
    {
        None,
        Start,
        End
    };
    ClimbHandleDrag m_climbHandleDrag = ClimbHandleDrag::None;
    double          m_dragOriginalClimbStartSec = -1.0;
    double          m_dragOriginalClimbEndSec = -1.0;
    bool            m_newClimbCreating = false;
    bool            m_newClimbDragging = false;
    double          m_newClimbStartSec = -1.0;
    double          m_newClimbCurrentSec = -1.0;
    double          m_dataMinY      = 0.0;   // raw min from ride data
    double          m_dataMaxY      = 1.0;   // raw max from ride data
};
