// SPDX-License-Identifier: GPL-3

#include "RouteColorizer.h"

namespace RouteColorizer
{
QColor gradientColorForSlope(double percent)
{
    if (percent <= -8.0) return QColor("#7c2d12");
    if (percent <= -4.0) return QColor("#ea580c");
    if (percent <= -1.0) return QColor("#f59e0b");
    if (percent < 1.0)    return QColor("#94a3b8");
    if (percent < 4.0)    return QColor("#22c55e");
    if (percent < 8.0)    return QColor("#16a34a");
    return QColor("#15803d");
}

ColorMetric routeModeToMetric(RouteColorMode mode)
{
    switch (mode)
    {
        case RouteColorMode::Power:     return ColorMetric::Power;
        case RouteColorMode::HeartRate: return ColorMetric::HeartRate;
        case RouteColorMode::Cadence:   return ColorMetric::Cadence;
        case RouteColorMode::Speed:     return ColorMetric::Speed;
        case RouteColorMode::Altitude:  return ColorMetric::Altitude;
        case RouteColorMode::None:
        case RouteColorMode::Gradient:
            return ColorMetric::None;
    }

    return ColorMetric::None;
}
}
