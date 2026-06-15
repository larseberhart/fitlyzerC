// SPDX-License-Identifier: GPL-3

/**
 * @file RideChartWidget.h
 * @brief Chart widget and visualization support for RideChartWidget.
 *
 * Provides chart rendering or chart-related data types used to visualize ride and fitness metrics in the UI.
 *
 * Responsibilities:
 * - Provide chart visualization behavior or chart support types
 *
 * @author Lars EBERHART
 */

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

/**
 * @brief Interactive chart displaying ride data by selected metric.
 *
 * Shows power, heart rate, cadence, speed, or altitude over time with overlays
 * for intervals, climbs, and zone visualization.
 */
class RideChartWidget : public QChartView
{
    Q_OBJECT

public:
    /**
     * @brief Constructs ride chart for specified metric.
     * @param metric Metric to display (Power, HeartRate, etc.).
     * @param parent Parent widget.
     */
    explicit RideChartWidget(Metric metric, QWidget* parent = nullptr);

    /**
     * @brief Sets ride data to display.
     * @param rideData Activity data.
     * @param colorMetric Metric to use for zone coloring.
     * @param colorContext Athlete context for coloring.
     */
    void setRideData(const RideData& rideData,
                     ColorMetric colorMetric = ColorMetric::None,
                     const ColorContext& colorContext = {});

    /**
     * @brief Sets power smoothing window.
     * @param seconds Smoothing window in seconds.
     */
    void setPowerSmoothingSeconds(int seconds);

    /**
     * @brief Enables/disables automatic smoothing.
     * @param enabled True to enable auto smoothing.
     */
    void setAutoSmoothingEnabled(bool enabled);

    /**
     * @brief Sets interval overlays.
     * @param intervals Work/recovery intervals.
     */
    void setIntervals(const std::vector<Interval>& intervals);

    /**
     * @brief Sets climb overlays.
     * @param climbs Detected climbs.
     */
    void setClimbs(const std::vector<Climb>& climbs);

    /**
     * @brief Enables/disables metric overlay.
     * @param metric Metric to overlay.
     * @param enabled True to show overlay.
     */
    void setMetricOverlay(ColorMetric metric, bool enabled);

    /**
     * @brief Highlights a specific climb.
     * @param index Climb index (-1 to clear).
     */
    void setSelectedClimbIndex(int index);

    /**
     * @brief Enables/disables climb editing mode.
     * @param enabled True to enable editing.
     */
    void setClimbEditingEnabled(bool enabled);

    /**
     * @brief Clears the chart.
     */
    void clearChart();

    /**
     * @brief Checks if chart has data.
     * @return True if data is displayed.
     */
    bool hasData() const { return m_hasData; }

    /**
     * @brief Gets visible range start time.
     * @return Start time in minutes.
     */
    double visibleRangeStartMinutes() const;
    /// @brief Gets end time of visible range in minutes from activity start.
    double visibleRangeEndMinutes() const;

public slots:
    /// @slot Sets visible x-axis range.
    /// @param min Minimum time in minutes.
    /// @param max Maximum time in minutes.
    void setXRange(double min, double max);

    /// @slot Sets crosshair position.
    /// @param xMinutes Time in minutes.
    void setCrosshair(double xMinutes);

    /// @slot Resets zoom to full ride extent.
    void resetZoom();

    /// @slot Alias for resetZoom().
    void fitToData();

signals:
    /// @signal Emitted when visible x range changes.
    /// @param min Minimum time in minutes.
    /// @param max Maximum time in minutes.
    void xRangeChanged(double min, double max);

    /// @signal Emitted when visible time range updates.
    /// @param startMinutes Start time in minutes.
    /// @param endMinutes End time in minutes.
    void visibleRangeChanged(double startMinutes, double endMinutes);

    /// @signal Emitted when crosshair moves.
    /// @param xMinutes Crosshair time in minutes.
    void crosshairMoved(double xMinutes);

    /// @signal Emitted when user selects time interval.
    /// @param startSeconds Start time in seconds from activity start.
    /// @param endSeconds End time in seconds from activity start.
    void intervalSelectionRequested(double startSeconds, double endSeconds);

    /// @signal Emitted when climb boundaries are edited.
    /// @param oldStartSeconds Original start time in seconds.
    /// @param oldEndSeconds Original end time in seconds.
    /// @param newStartSeconds New start time in seconds.
    /// @param newEndSeconds New end time in seconds.
    void climbBoundaryEdited(
        double oldStartSeconds,
        double oldEndSeconds,
        double newStartSeconds,
        double newEndSeconds);

    /// @signal Emitted when user creates new climb.
    /// @param startSeconds Start time in seconds.
    /// @param endSeconds End time in seconds.
    void newClimbRequested(double startSeconds, double endSeconds);

    /// @signal Emitted when climb is clicked.
    /// @param climbIndex Index of clicked climb.
    void climbClicked(int climbIndex);

protected:
    /// @brief Handles wheel scroll for panning and zooming.
    void wheelEvent(QWheelEvent* event) override;

    /// @brief Handles mouse press for chart interaction.
    void mousePressEvent(QMouseEvent* event) override;

    /// @brief Handles mouse release for chart interaction.
    void mouseReleaseEvent(QMouseEvent* event) override;

    /// @brief Handles double-click for interval creation.
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    /// @brief Handles mouse movement for crosshair and selection.
    void mouseMoveEvent(QMouseEvent* event) override;

    /// @brief Handles mouse leaving widget.
    void leaveEvent(QEvent* event) override;

    /// @brief Renders chart foreground elements.
    void drawForeground(QPainter* painter, const QRectF& rect) override;

    /// @brief Handles widget resize events.
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

    // Cache for Y-axis range calculations during zoom to avoid redundant recalculation
    mutable struct
    {
        double cachedMinX = -1.0;
        double cachedMaxX = -1.0;
        double cachedAxisMin = 0.0;
        double cachedAxisMax = 1.0;
        bool valid = false;
    } m_yAxisRangeCache;
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