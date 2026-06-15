// SPDX-License-Identifier: GPL-3

/**
 * @file PowerHistogramWidget.h
 * @brief Chart widget and visualization support for PowerHistogramWidget.
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

#include "fit/RideData.h"

/**
 * @brief Displays power distribution histogram.
 *
 * Shows frequency distribution of power output across power range.
 */
class PowerHistogramWidget : public QChartView
{
    Q_OBJECT
public:
    /**
     * @brief Constructs power histogram chart widget.
     * @param parent Parent widget.
     */
    explicit PowerHistogramWidget(QWidget* parent = nullptr);

    /**
     * @brief Sets ride data to display.
     * @param rideData Activity data.
     */
    void setRideData(const RideData& rideData);

    /**
     * @brief Clears the chart.
     */
    void clearChart();
};