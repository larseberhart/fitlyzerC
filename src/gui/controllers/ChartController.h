// SPDX-License-Identifier: GPL-3

#pragma once

#include <QObject>
#include <memory>

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

private:
    /// @brief Helper to connect chart widget signals for synchronization.
    void connectChartSignals();

    /// @brief Helper to apply current color metric to charts.
    void applyColorMetric();

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
};
