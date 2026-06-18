// SPDX-License-Identifier: GPL-3

#include "RideChartRenderer.h"

#include <cmath>

namespace RideChartRenderer
{
double niceStep(double range)
{
    if (range <= 0.0) return 1.0;
    const double rough     = range / 8.0;
    const double magnitude = std::pow(10.0, std::floor(std::log10(rough)));
    const double residual  = rough / magnitude;
    double step;
    if      (residual < 1.5) step = 1.0;
    else if (residual < 3.5) step = 2.0;
    else if (residual < 7.5) step = 5.0;
    else                     step = 10.0;
    return step * magnitude;
}

void niceRange(double dataMin, double dataMax,
               double& axisMin, double& axisMax)
{
    const double range = dataMax - dataMin;
    const double step  = niceStep(range > 0.0 ? range : dataMax);
    axisMin = (dataMin < step) ? 0.0 : std::floor(dataMin / step) * step;
    axisMax = std::ceil (dataMax / step) * step;
    if (axisMax <= axisMin) axisMax = axisMin + step;
}
}
