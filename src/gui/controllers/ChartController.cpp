// SPDX-License-Identifier: GPL-3

#include "ChartController.h"

#include "../WorkoutController.h"
#include "../../database/DatabaseManager.h"
#include "charts/RideChartWidget.h"
#include "charts/PowerCurveWidget.h"
#include "charts/PowerHistogramWidget.h"
#include "charts/FitnessChartWidget.h"
#include "core/zones/ZoneDefinition.h"

#include <QTabWidget>
#include <QStackedLayout>
#include <QTableWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QLayout>

/**
 * @brief Constructs the chart controller.
 * @param controller Pointer to WorkoutController for data access.
 * @param dbManager Pointer to DatabaseManager for queries.
 * @param parent Parent object.
 */
ChartController::ChartController(
    WorkoutController* controller,
    DatabaseManager* dbManager,
    QObject* parent)
    : QObject(parent)
    , m_controller(controller)
    , m_dbManager(dbManager)
{
    if (m_controller)
    {
        connect(m_controller, &WorkoutController::workoutLoaded,
                this, &ChartController::onWorkoutLoaded);
    }
}

/**
 * @brief Destructor.
 */
ChartController::~ChartController() = default;

/**
 * @brief Sets the chart widgets managed by this controller.
 */
void ChartController::setChartWidgets(
    RideChartWidget* powerChart,
    RideChartWidget* hrChart,
    RideChartWidget* cadenceChart,
    RideChartWidget* speedChart,
    RideChartWidget* altitudeChart,
    PowerHistogramWidget* histogram,
    PowerCurveWidget* powerCurve,
    FitnessChartWidget* fitnessChart)
{
    m_powerChart = powerChart;
    m_hrChart = hrChart;
    m_cadenceChart = cadenceChart;
    m_speedChart = speedChart;
    m_altitudeChart = altitudeChart;
    m_histogram = histogram;
    m_powerCurve = powerCurve;
    m_fitnessChart = fitnessChart;

    connectChartSignals();
}

/**
 * @brief Sets the analysis tab-related widgets.
 */
void ChartController::setAnalysisTabWidgets(
    QTabWidget* analysisTabWidget,
    ColorLegendWidget* colorLegend,
    QStackedLayout* activityTabStack,
    QStackedLayout* zonesTabStack,
    QStackedLayout* histogramTabStack,
    QStackedLayout* powerCurveTabStack,
    QStackedLayout* calendarTabStack,
    QStackedLayout* fitnessTabStack)
{
    m_analysisTabWidget = analysisTabWidget;
    m_colorLegend = colorLegend;
    m_activityTabStack = activityTabStack;
    m_zonesTabStack = zonesTabStack;
    m_histogramTabStack = histogramTabStack;
    m_powerCurveTabStack = powerCurveTabStack;
    m_calendarTabStack = calendarTabStack;
    m_fitnessTabStack = fitnessTabStack;
}

/**
 * @brief Sets the zones table widget for zone distribution display.
 */
void ChartController::setZonesTable(QTableWidget* zonesTable)
{
    m_zonesTable = zonesTable;
}

/**
 * @brief Sets the climb chart widgets.
 */
void ChartController::setClimbCharts(
    RideChartWidget* climbPowerChart,
    RideChartWidget* climbHrChart,
    RideChartWidget* climbCadenceChart,
    RideChartWidget* climbSpeedChart,
    RideChartWidget* climbAltitudeChart)
{
    m_climbPowerChart = climbPowerChart;
    m_climbHrChart = climbHrChart;
    m_climbCadenceChart = climbCadenceChart;
    m_climbSpeedChart = climbSpeedChart;
    m_climbAltitudeChart = climbAltitudeChart;
}

/**
 * @brief Sets the chart control widgets.
 */
void ChartController::setChartControls(
    QComboBox* colorMetricCombo,
    QComboBox* powerSmoothingCombo,
    QCheckBox* autoSmoothingCheck,
    QCheckBox* climbOverlayEnabledCheck,
    QComboBox* climbOverlayMetricCombo,
    QLayout* colorLegendLayout)
{
    m_colorMetricCombo = colorMetricCombo;
    m_powerSmoothingCombo = powerSmoothingCombo;
    m_autoSmoothingCheck = autoSmoothingCheck;
    m_climbOverlayEnabledCheck = climbOverlayEnabledCheck;
    m_climbOverlayMetricCombo = climbOverlayMetricCombo;
    m_colorLegendLayout = colorLegendLayout;
}

/**
 * @brief Sets athlete context for power curve queries.
 */
void ChartController::setAthleteId(int athleteId)
{
    m_currentAthleteId = athleteId;
}

/**
 * @brief Updates all managed charts with current activity data.
 */
void ChartController::updateCharts()
{
    if (!m_controller)
        return;

    const int colorMetricValue = m_colorMetricCombo ? m_colorMetricCombo->currentData().toInt() : 0;
    const ColorMetric colorMetric = static_cast<ColorMetric>(colorMetricValue);

    if (!m_powerChart)
        return;

    // Determine which climbs to show (filtered or all)
    const std::vector<Climb>& climbsForView = m_climbs.empty() ? m_controller->climbs() : m_climbs;

    // Apply smoothing settings first
    applyChartSmoothing();

    // Disable updates during population
    auto* parentWidget = qobject_cast<QWidget*>(parent());
    if (parentWidget)
        parentWidget->setUpdatesEnabled(false);

    // Update main ride charts
    m_powerChart->setIntervals(m_controller->intervals());
    m_powerChart->setClimbs(climbsForView);
    if (m_hrChart) m_hrChart->setClimbs(climbsForView);
    if (m_cadenceChart) m_cadenceChart->setClimbs(climbsForView);
    if (m_speedChart) m_speedChart->setClimbs(climbsForView);
    if (m_altitudeChart) m_altitudeChart->setClimbs(climbsForView);

    m_powerChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);
    if (m_hrChart) m_hrChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);
    if (m_cadenceChart) m_cadenceChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);
    if (m_speedChart) m_speedChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);
    if (m_altitudeChart) m_altitudeChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);

    // Update climb-specific charts if available
    if (m_climbPowerChart)
    {
        m_climbPowerChart->setClimbs(climbsForView);
        if (m_climbHrChart) m_climbHrChart->setClimbs(climbsForView);
        if (m_climbCadenceChart) m_climbCadenceChart->setClimbs(climbsForView);
        if (m_climbSpeedChart) m_climbSpeedChart->setClimbs(climbsForView);
        if (m_climbAltitudeChart) m_climbAltitudeChart->setClimbs(climbsForView);

        m_climbPowerChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);
        if (m_climbHrChart) m_climbHrChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);
        if (m_climbCadenceChart) m_climbCadenceChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);
        if (m_climbSpeedChart) m_climbSpeedChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);
        if (m_climbAltitudeChart) m_climbAltitudeChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);

        // Apply climb overlay if enabled
        if (m_climbAltitudeChart && m_climbOverlayEnabledCheck && m_climbOverlayMetricCombo)
        {
            const ColorMetric overlayMetric = static_cast<ColorMetric>(m_climbOverlayMetricCombo->currentData().toInt());
            m_climbAltitudeChart->setMetricOverlay(
                overlayMetric,
                m_climbOverlayEnabledCheck->isChecked());
        }
    }

    if (parentWidget)
        parentWidget->setUpdatesEnabled(true);

    emit chartsUpdated();
}

/**
 * @brief Refreshes chart display without reloading data.
 */
void ChartController::refreshCharts()
{
    // TODO: Extract chart refresh logic from MainWindow
    emit chartsUpdated();
}

/**
 * @brief Resets zoom on all charts to fit data.
 */
void ChartController::resetAllZoom()
{
    if (m_powerChart) m_powerChart->fitToData();
    if (m_hrChart) m_hrChart->fitToData();
    if (m_cadenceChart) m_cadenceChart->fitToData();
    if (m_speedChart) m_speedChart->fitToData();
    if (m_altitudeChart) m_altitudeChart->fitToData();
}

/**
 * @brief Updates power histogram display.
 */
void ChartController::updateHistogram()
{
    // TODO: Extract updateHistogram logic from MainWindow
}

/**
 * @brief Updates fitness chart display.
 */
void ChartController::updateFitnessChart()
{
    // TODO: Extract updateFitnessChart logic from MainWindow
}

/**
 * @brief Updates empty state indicators for all chart tabs.
 */
void ChartController::updateAnalysisEmptyStates()
{
    // TODO: Extract updateAnalysisEmptyStates logic from MainWindow
}

/**
 * @brief Synchronizes chart zoom levels across all ride charts.
 */
void ChartController::syncChartZoom()
{
    // TODO: Extract chart zoom sync from MainWindow
}

/**
 * @brief Synchronizes crosshair position across all ride charts.
 */
void ChartController::syncChartCrosshair()
{
    // TODO: Extract crosshair sync from MainWindow
}

/**
 * @brief Helper to connect chart widget signals for synchronization.
 */
void ChartController::connectChartSignals()
{
    // TODO: Extract signal connections from MainWindow buildUI
}

/**
 * @brief Helper to update color legend display.
 */
void ChartController::updateColorLegendDisplay()
{
    // TODO: Extract color legend update from MainWindow
}

/**
 * @brief Retrieves current color metric from stored value.
 */
int ChartController::currentColorMetric() const
{
    return m_colorMetric;
}

/**
 * @brief Sets intervals for display on power chart.
 */
void ChartController::setIntervals(const std::vector<Interval>& intervals)
{
    m_intervals = intervals;
}

/**
 * @brief Sets climbs for display on charts.
 */
void ChartController::setClimbs(const std::vector<Climb>& climbs)
{
    m_climbs = climbs;
}

/**
 * @brief Sets the color metric for chart visualization.
 */
void ChartController::setColorMetric(int colorMetric)
{
    m_colorMetric = colorMetric;
}

/**
 * @brief Sets the color context for chart rendering.
 */
void ChartController::setColorContext(const ColorContext& colorContext)
{
    m_colorContext = colorContext;
}

/**
 * @brief Updates zone availability display.
 */
void ChartController::updateZoneAvailability()
{
    if (!m_analysisTabWidget)
        return;

    const bool hasPower = m_controller && m_controller->statistics().maximumPower > 0.0;

    m_analysisTabWidget->setTabEnabled(2, hasPower);  // Histogram tab
    m_analysisTabWidget->setTabEnabled(3, hasPower);  // PDC tab
    m_analysisTabWidget->setTabEnabled(4, true);      // Calendar tab
    m_analysisTabWidget->setTabEnabled(5, true);      // Fitness tab
}

/**
 * @brief Slot for when WorkoutController loads a new activity.
 */
void ChartController::onWorkoutLoaded()
{
    // Coordinate chart updates
    const bool hasPower = m_controller && m_controller->statistics().maximumPower > 0.0;
    if (hasPower)
    {
        updateHistogram();
        updateFitnessChart();
    }

    resetAllZoom();
    emit chartsUpdated();
}

/**
 * @brief Helper to apply current color metric to charts.
 */
void ChartController::applyColorMetric()
{
    // TODO: Extract color metric application from MainWindow
}

/**
 * @brief Updates zones table with zone distribution for current activity.
 */
void ChartController::updateZonesTab()
{
    if (!m_zonesTable || !m_controller || !m_dbManager)
        return;

    // Clear existing rows
    m_zonesTable->setRowCount(0);

    // TODO: Extract zone distribution logic from MainWindow
    // This includes computing zone hits and populating table with color-coded zones
}

/**
 * @brief Updates power curve display with athlete's historical power data.
 */
void ChartController::updatePowerCurve()
{
    if (!m_powerCurve || !m_controller || !m_dbManager || m_currentAthleteId < 0)
        return;

    // TODO: Extract power curve computation logic from MainWindow
    // This includes querying activities, computing power curves (90d, YTD, All-time)
    // and updating the power curve widget
}

/**
 * @brief Updates color legend widget with current zone colors.
 */
void ChartController::updateColorLegend()
{
    if (!m_colorLegend)
        return;

    // TODO: Extract color legend logic from MainWindow
    // This includes populating legend based on current color metric and zones
}

/**
 * @brief Helper to apply smoothing settings to all charts.
 */
void ChartController::applyChartSmoothing()
{
    const std::initializer_list<RideChartWidget*> all =
        { m_powerChart, m_hrChart, m_cadenceChart, m_speedChart, m_altitudeChart,
          m_climbPowerChart, m_climbHrChart, m_climbCadenceChart, m_climbSpeedChart, m_climbAltitudeChart };

    const int smoothingSeconds = m_powerSmoothingCombo ? m_powerSmoothingCombo->currentData().toInt() : 0;
    const bool autoSmoothing = m_autoSmoothingCheck ? m_autoSmoothingCheck->isChecked() : false;

    for (auto* chart : all)
    {
        if (!chart)
            continue;
        chart->setPowerSmoothingSeconds(smoothingSeconds);
        chart->setAutoSmoothingEnabled(autoSmoothing);
    }
}
