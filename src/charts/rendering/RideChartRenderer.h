// SPDX-License-Identifier: GPL-3

#pragma once

namespace RideChartRenderer
{
double niceStep(double range);
void niceRange(double dataMin, double dataMax,
               double& axisMin, double& axisMax);
}
