// SPDX-License-Identifier: GPL-3


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