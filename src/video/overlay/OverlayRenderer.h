// SPDX-License-Identifier: GPL-3

#pragma once

#include <QString>

#include "core/zones/ZoneDefinition.h"

namespace OverlayRenderer
{
QString formatDuration(double seconds);
QString formatLegendValue(double value, ColorMetric metric);
QString zoneLegendText(const Zone& zone, ColorMetric metric);
}
