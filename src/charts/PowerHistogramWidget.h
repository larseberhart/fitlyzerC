#pragma once

#include <QtCharts/QChartView>

#include "fit/RideData.h"

class PowerHistogramWidget : public QChartView
{
    Q_OBJECT
public:
    explicit PowerHistogramWidget(QWidget* parent = nullptr);
    void setRideData(const RideData& rideData);
    void clearChart();
};
