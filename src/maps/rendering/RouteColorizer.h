// SPDX-License-Identifier: GPL-3

#pragma once

#include <QColor>

#include "maps/MapRenderer.h"

namespace RouteColorizer
{
QColor gradientColorForSlope(double percent);
ColorMetric routeModeToMetric(RouteColorMode mode);
}
