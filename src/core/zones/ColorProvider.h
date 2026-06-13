#pragma once

#include <QColor>

#include <vector>

#include "ZoneDefinition.h"

class ColorProvider
{
public:
    static QColor colorForMetric(ColorMetric metric,
                                 double value,
                                 const ColorContext& context);
    static QColor colorForPower(double watts, int ftp);
    static QColor colorForHeartRate(double bpm,
                                    int heartRateMax,
                                    int heartRateRest = 60,
                                    int thresholdHeartRate = 0);
    static QColor colorForCadence(double cadence);
    static QColor colorForSpeed(double speedKmh);
    static QColor colorForAltitude(double altitude,
                                   double minAltitude,
                                   double maxAltitude);

    static QColor colorForValue(double value,
                                const std::vector<Zone>& zones,
                                const QColor& fallback = QColor("#94a3b8"));
};