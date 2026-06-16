// SPDX-License-Identifier: GPL-3


#include "ZoneDefinition.h"

QString colorMetricDisplayName(ColorMetric metric)
{
    switch (metric)
    {
        case ColorMetric::None:      return "None";
        case ColorMetric::Power:     return "Power";
        case ColorMetric::HeartRate: return "Heart Rate";
        case ColorMetric::Cadence:   return "Cadence";
        case ColorMetric::Speed:     return "Speed";
        case ColorMetric::Altitude:  return "Altitude";
        case ColorMetric::Gradient:  return "Gradient";
    }

    return "None";
}

QString colorMetricUnit(ColorMetric metric)
{
    switch (metric)
    {
        case ColorMetric::Power:     return "W";
        case ColorMetric::HeartRate: return "bpm";
        case ColorMetric::Cadence:   return "rpm";
        case ColorMetric::Speed:     return "km/h";
        case ColorMetric::Altitude:  return "m";
        case ColorMetric::Gradient:  return {};
        case ColorMetric::None:      return {};
    }

    return {};
}