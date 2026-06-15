// SPDX-License-Identifier: GPL-3

/**
 * @file PowerCurveWidget.h
 * @brief Chart widget and visualization support for PowerCurveWidget.
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

#include "analysis/PowerCurve.h"
#include "fit/RideData.h"

/**
 * @brief Displays power output over duration curve.
 *
 * Shows best mean power at various durations with optional comparison curves
 * (last 90 days, current year, all-time).
 */
class PowerCurveWidget : public QChartView
{
    Q_OBJECT
public:
    /**
     * @brief Constructs power curve chart widget.
     * @param parent Parent widget.
     */
    explicit PowerCurveWidget(QWidget* parent = nullptr);

    /**
     * @brief Sets ride data to display.
     * @param rideData Activity data.
     * @param ftp Functional threshold power (default 0).
     */
    void setRideData(const RideData& rideData, double ftp = 0.0);

    /**
     * @brief Sets ride data with comparison curves.
     * @param rideData Current activity data.
     * @param last90 Power curve for last 90 days.
     * @param currentYear Power curve for current year.
     * @param allTime Power curve for all time.
     * @param ftp Functional threshold power (default 0).
     */
    void setRideDataWithComparisons(const RideData& rideData,
                                    const std::vector<PowerCurvePoint>& last90,
                                    const std::vector<PowerCurvePoint>& currentYear,
                                    const std::vector<PowerCurvePoint>& allTime,
                                    double ftp = 0.0);

    /**
     * @brief Clears the chart.
     */
    void clearChart();
};