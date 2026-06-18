// SPDX-License-Identifier: GPL-3

#pragma once

#include <QObject>
#include <memory>
#include <vector>
#include "core/zones/ZoneDefinition.h"

class WorkoutController;
class DatabaseManager;
class RideChartWidget;
class PowerCurveWidget;
class PowerHistogramWidget;
class FitnessChartWidget;

/**
 * @brief Manages chart widgets and their synchronization.
 *
 * Handles:
 * - RideChartWidget (power, HR, cadence, speed, altitude)
 * - PowerCurveWidget
 * - PowerHistogramWidget
 * - FitnessChartWidget
 * - Chart updates and refresh
 * - Chart synchronization (zoom, crosshair, selection)
 * - Chart zoom handling
 */
class ChartController : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the chart controller.
     * @param controller Pointer to WorkoutController for data access.
     * @param dbManager Pointer to DatabaseManager for queries.
     * @param parent Parent object.
     */
    explicit ChartController(
        WorkoutController* controller,
        DatabaseManager* dbManager,
        QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~ChartController() override;

    /**
     * @brief Sets the chart widgets managed by this controller.
     * @param powerChart Power chart widget.
     * @param hrChart Heart rate chart widget.
     * @param cadenceChart Cadence chart widget.
     * @param speedChart Speed chart widget.
     * @param altitudeChart Altitude chart widget.
     * @param histogram Power histogram widget.
     * @param powerCurve Power duration curve widget.
     * @param fitnessChart Fitness/fatigue/form chart widget.
     */
    void setChartWidgets(
        RideChartWidget* powerChart,
        RideChartWidget* hrChart,
        RideChartWidget* cadenceChart,
        RideChartWidget* speedChart,
        RideChartWidget* altitudeChart,
        PowerHistogramWidget* histogram,
        PowerCurveWidget* powerCurve,
        FitnessChartWidget* fitnessChart);

    /**
     * @brief Sets the analysis tab-related widgets.
     * @param analysisTabWidget Main analysis tab widget.
     * @param colorLegend Color legend widget.
     * @param activityTabStack Stacked layout for activity tab.
     * @param zonesTabStack Stacked layout for zones tab.
     * @param histogramTabStack Stacked layout for histogram tab.
     * @param powerCurveTabStack Stacked layout for power curve tab.
     * @param calendarTabStack Stacked layout for calendar tab.
     * @param fitnessTabStack Stacked layout for fitness tab.
     */
    void setAnalysisTabWidgets(
        class QTabWidget* analysisTabWidget,
        class ColorLegendWidget* colorLegend,
        class QStackedLayout* activityTabStack,
        class QStackedLayout* zonesTabStack,
        class QStackedLayout* histogramTabStack,
        class QStackedLayout* powerCurveTabStack,
        class QStackedLayout* calendarTabStack,
        class QStackedLayout* fitnessTabStack);

    /**
     * @brief Updates all managed charts with current activity data.
     */
    void updateCharts();

    /**
     * @brief Refreshes chart display without reloading data.
     */
    void refreshCharts();

    /**
     * @brief Resets zoom on all charts to fit data.
     */
    void resetAllZoom();

    /**
     * @brief Updates zones tab display.
     */
    void updateZonesTab();

    /**
     * @brief Updates power histogram display.
     */
    void updateHistogram();

    /**
     * @brief Updates power curve display.
     */
    void updatePowerCurve();

    /**
     * @brief Updates fitness chart display.
     */
    void updateFitnessChart();

    /**
     * @brief Updates empty state indicators for all chart tabs.
     */
    void updateAnalysisEmptyStates();

    /**
     * @brief Sets intervals for display on power chart.
     * @param intervals List of intervals to display.
     */
    void setIntervals(const std::vector<class Interval>& intervals);

    /**
     * @brief Sets climbs for display on charts.
     * @param climbs List of climbs to display.
     */
    void setClimbs(const std::vector<class Climb>& climbs);

    /**
     * @brief Sets the color metric for chart visualization.
     * @param colorMetric Color metric enumeration value.
     */
    void setColorMetric(int colorMetric);

    /**
     * @brief Sets the color context for chart rendering.
     * @param colorContext Color context structure.
     */
    void setColorContext(const class ColorContext& colorContext);

    /**
     * @brief Updates the color legend display.
     */
    void updateColorLegend();

    /**
     * @brief Updates zone availability display.
     */
    void updateZoneAvailability();

    /**
     * @brief Synchronizes chart zoom levels across all ride charts.
     */
    void syncChartZoom();

    /**
     * @brief Synchronizes crosshair position across all ride charts.
     */
    void syncChartCrosshair();

signals:
    /// @brief Emitted when charts have been successfully updated.
    void chartsUpdated();

    /// @brief Emitted when an error occurs during chart operations.
    void errorOccurred(const QString& message);

private slots:
    /// @brief Slot for when WorkoutController loads a new activity.
    void onWorkoutLoaded();

private:
    /// @brief Helper to connect chart widget signals for synchronization.
    void connectChartSignals();

    /// @brief Helper to apply current color metric to charts.
    void applyColorMetric();

    /// @brief Helper to update color legend display.
    void updateColorLegendDisplay();

    /// @brief Retrieves current color metric from stored value.
    int currentColorMetric() const;

    WorkoutController* m_controller;
    DatabaseManager* m_dbManager;

    RideChartWidget* m_powerChart = nullptr;
    RideChartWidget* m_hrChart = nullptr;
    RideChartWidget* m_cadenceChart = nullptr;
    RideChartWidget* m_speedChart = nullptr;
    RideChartWidget* m_altitudeChart = nullptr;
    PowerHistogramWidget* m_histogram = nullptr;
    PowerCurveWidget* m_powerCurve = nullptr;
    FitnessChartWidget* m_fitnessChart = nullptr;
    class QStackedLayout* m_activityTabStack = nullptr;
    class QStackedLayout* m_zonesTabStack = nullptr;
    class QStackedLayout* m_histogramTabStack = nullptr;
    class QStackedLayout* m_powerCurveTabStack = nullptr;
    class QStackedLayout* m_calendarTabStack = nullptr;
    class QStackedLayout* m_fitnessTabStack = nullptr;
    class QTabWidget* m_analysisTabWidget = nullptr;
    class ColorLegendWidget* m_colorLegend = nullptr;

    // Chart data
    std::vector<class Interval> m_intervals;
    std::vector<class Climb> m_climbs;
    int m_colorMetric = 0;
    class ColorContext m_colorContext;
    bool m_smoothingEnabled = false;
    int m_smoothingAmount = 0;
};
