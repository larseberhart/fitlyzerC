#pragma once

#include <QtCharts/QChartView>

#include <vector>

#include "analysis/TrainingLoad.h"

class QLineSeries;
class QValueAxis;

class FitnessChartWidget : public QChartView
{
    Q_OBJECT

public:
    explicit FitnessChartWidget(QWidget* parent = nullptr);

    void setTimeline(const std::vector<FitnessMetricsPoint>& timeline);
    void clearChart();

private:
    QLineSeries* m_ctlSeries = nullptr;
    QLineSeries* m_atlSeries = nullptr;
    QLineSeries* m_tsbSeries = nullptr;
    QValueAxis* m_axisX = nullptr;
    QValueAxis* m_axisY = nullptr;
};
