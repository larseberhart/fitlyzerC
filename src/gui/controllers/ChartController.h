// SPDX-License-Identifier: GPL-3

#pragma once

#include <vector>

#include <QObject>
#include <QString>

#include "core/zones/ZoneDefinition.h"

class WorkoutController;
class DatabaseManager;
class RideChartWidget;
class PowerCurveWidget;
class PowerHistogramWidget;
class FitnessChartWidget;
class ColorLegendWidget;
class QTabWidget;
class QStackedLayout;
class QTableWidget;
class QComboBox;
class QCheckBox;
class QLayout;
class QWidget;
class Interval;
class Climb;

/**
 * @brief Manages all chart widgets in the analysis view.
 */
class ChartController : public QObject
{
    Q_OBJECT

public:
    explicit ChartController(
        WorkoutController* controller,
        DatabaseManager* dbManager,
        QObject* parent = nullptr);
    ~ChartController() override;

    /**
     * @brief Sets the main ride chart widgets managed by this controller.
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
     * @brief Sets the analysis tab container and layout widgets.
     */
    void setAnalysisTabWidgets(
        QTabWidget* analysisTabWidget,
        ColorLegendWidget* colorLegend,
        QStackedLayout* activityTabStack,
        QStackedLayout* zonesTabStack,
        QStackedLayout* histogramTabStack,
        QStackedLayout* powerCurveTabStack,
        QStackedLayout* calendarTabStack,
        QStackedLayout* fitnessTabStack);

    /**
     * @brief Sets the zones table widget for zone distribution display.
     */
    void setZonesTable(QTableWidget* zonesTable);

    /**
     * @brief Sets the climb chart widgets.
     */
    void setClimbCharts(
        RideChartWidget* climbPowerChart,
        RideChartWidget* climbHrChart,
        RideChartWidget* climbCadenceChart,
        RideChartWidget* climbSpeedChart,
        RideChartWidget* climbAltitudeChart);

    /**
     * @brief Sets the chart control widgets.
     */
    void setChartControls(
        QComboBox* colorMetricCombo,
        QComboBox* powerSmoothingCombo,
        QCheckBox* autoSmoothingCheck,
        QCheckBox* climbOverlayEnabledCheck,
        QComboBox* climbOverlayMetricCombo,
        QLayout* colorLegendLayout);

    /**
     * @brief Sets athlete context for power curve queries.
     */
    void setAthleteId(int athleteId);

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
     * @brief Updates power histogram display.
     */
    void updateHistogram();

    /**
     * @brief Updates fitness chart display.
     */
    void updateFitnessChart();

    /**
     * @brief Updates empty state indicators for all chart tabs.
     */
    void updateAnalysisEmptyStates();

    /**
     * @brief Updates zones table with zone distribution.
     */
    void updateZonesTab();

    /**
     * @brief Updates power curve with historical athlete data.
     */
    void updatePowerCurve();

    /**
     * @brief Updates color legend with zone colors.
     */
    void updateColorLegend();

    /**
     * @brief Applies chart smoothing settings to all charts.
     */
    void applyChartSmoothing();

    /**
     * @brief Synchronizes chart zoom levels across all ride charts.
     */
    void syncChartZoom();

    /**
     * @brief Synchronizes crosshair position across all ride charts.
     */
    void syncChartCrosshair();

    /**
     * @brief Sets intervals for display on power chart.
     * @param intervals List of intervals to display.
     */
    void setIntervals(const std::vector<Interval>& intervals);

    /**
     * @brief Sets climbs for display on charts.
     * @param climbs List of climbs to display.
     */
    void setClimbs(const std::vector<Climb>& climbs);

    /**
     * @brief Sets the color metric for chart visualization.
     * @param colorMetric Color metric enumeration value.
     */
    void setColorMetric(int colorMetric);

    /**
     * @brief Sets the color context for chart rendering.
     * @param colorContext Color context structure.
     */
    void setColorContext(const ColorContext& colorContext);

    /**
     * @brief Updates zone availability display.
     */
    void updateZoneAvailability();

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

    // Data members
    WorkoutController* m_controller;
    DatabaseManager* m_dbManager;

    // Main chart widgets
    RideChartWidget* m_powerChart = nullptr;
    RideChartWidget* m_hrChart = nullptr;
    RideChartWidget* m_cadenceChart = nullptr;
    RideChartWidget* m_speedChart = nullptr;
    RideChartWidget* m_altitudeChart = nullptr;
    PowerHistogramWidget* m_histogram = nullptr;
    PowerCurveWidget* m_powerCurve = nullptr;
    FitnessChartWidget* m_fitnessChart = nullptr;

    // Analysis tab widgets
    QTabWidget* m_analysisTabWidget = nullptr;
    ColorLegendWidget* m_colorLegend = nullptr;
    QStackedLayout* m_activityTabStack = nullptr;
    QStackedLayout* m_zonesTabStack = nullptr;
    QStackedLayout* m_histogramTabStack = nullptr;
    QStackedLayout* m_powerCurveTabStack = nullptr;
    QStackedLayout* m_calendarTabStack = nullptr;
    QStackedLayout* m_fitnessTabStack = nullptr;

    // Chart data
    std::vector<Interval> m_intervals;
    std::vector<Climb> m_climbs;
    int m_colorMetric = 0;
    ColorContext m_colorContext;
    bool m_smoothingEnabled = false;
    int m_smoothingAmount = 0;

    // Additional chart widgets
    RideChartWidget* m_climbPowerChart = nullptr;
    RideChartWidget* m_climbHrChart = nullptr;
    RideChartWidget* m_climbCadenceChart = nullptr;
    RideChartWidget* m_climbSpeedChart = nullptr;
    RideChartWidget* m_climbAltitudeChart = nullptr;
    QTableWidget* m_zonesTable = nullptr;

    // Chart controls
    QComboBox* m_colorMetricCombo = nullptr;
    QComboBox* m_powerSmoothingCombo = nullptr;
    QCheckBox* m_autoSmoothingCheck = nullptr;
    QCheckBox* m_climbOverlayEnabledCheck = nullptr;
    QComboBox* m_climbOverlayMetricCombo = nullptr;
    QLayout* m_colorLegendLayout = nullptr;

    // Athlete context
    int m_currentAthleteId = -1;
};
