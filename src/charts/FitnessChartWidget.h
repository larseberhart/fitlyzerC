// SPDX-License-Identifier: GPL-3


#pragma once

#include <QtCharts/QChartView>

#include <vector>

#include "analysis/TrainingLoad.h"

class QLineSeries;
class QValueAxis;

/**
 * @brief Displays fitness metrics timeline (CTL, ATL, TSB).
 *
 * Shows cumulative training load, acute training load, and training stress balance
 * over a date range using a line chart.
 */
class FitnessChartWidget : public QChartView
{
    Q_OBJECT

public:
    /**
     * @brief Constructs fitness chart widget.
     * @param parent Parent widget.
     */
    explicit FitnessChartWidget(QWidget* parent = nullptr);

    /**
     * @brief Sets fitness metrics timeline to display.
     * @param timeline Timeline of fitness metrics.
     */
    void setTimeline(const std::vector<FitnessMetricsPoint>& timeline);

    /**
     * @brief Clears the chart.
     */
    void clearChart();

private:
    QLineSeries* m_ctlSeries = nullptr;
    QLineSeries* m_atlSeries = nullptr;
    QLineSeries* m_tsbSeries = nullptr;
    QValueAxis* m_axisX = nullptr;
    QValueAxis* m_axisY = nullptr;
};