// SPDX-License-Identifier: GPL-3

#include "ChartController.h"

#include "../WorkoutController.h"
#include "../../database/DatabaseManager.h"
#include "charts/RideChartWidget.h"
#include "charts/PowerCurveWidget.h"
#include "charts/PowerHistogramWidget.h"
#include "charts/FitnessChartWidget.h"

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
 * @brief Updates all managed charts with current activity data.
 */
void ChartController::updateCharts()
{
    // TODO: Extract updateCharts logic from MainWindow
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
 * @brief Updates zones tab display.
 */
void ChartController::updateZonesTab()
{
    // TODO: Extract updateZonesTab logic from MainWindow
}

/**
 * @brief Updates power histogram display.
 */
void ChartController::updateHistogram()
{
    // TODO: Extract updateHistogram logic from MainWindow
}

/**
 * @brief Updates power curve display.
 */
void ChartController::updatePowerCurve()
{
    // TODO: Extract updatePowerCurve logic from MainWindow
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
 * @brief Helper to apply current color metric to charts.
 */
void ChartController::applyColorMetric()
{
    // TODO: Extract color metric application from MainWindow
}
