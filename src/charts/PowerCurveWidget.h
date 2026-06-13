#pragma once

#include <QtCharts/QChartView>

#include "analysis/PowerCurve.h"
#include "fit/RideData.h"

class PowerCurveWidget : public QChartView
{
    Q_OBJECT
public:
    explicit PowerCurveWidget(QWidget* parent = nullptr);
    void setRideData(const RideData& rideData, double ftp = 0.0);
    void setRideDataWithComparisons(const RideData& rideData,
                                    const std::vector<PowerCurvePoint>& last90,
                                    const std::vector<PowerCurvePoint>& currentYear,
                                    const std::vector<PowerCurvePoint>& allTime,
                                    double ftp = 0.0);
    void clearChart();
};
