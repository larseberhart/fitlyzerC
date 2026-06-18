// SPDX-License-Identifier: GPL-3

#pragma once

#include <QColor>
#include <QString>

#include "charts/Metric.h"
#include "core/zones/ZoneDefinition.h"
#include "fit/RideData.h"

namespace RideChartDataBuilder
{
QString labelForMetric(Metric metric);
QString labelForColorMetric(ColorMetric metric);
Metric metricFromColorMetric(ColorMetric metric);
double valueForMetric(const RideRecord& record, Metric metric);
bool isValidSample(const RideRecord& record, Metric metric);
int autoSmoothingSecondsForSpan(double spanMinutes);
QColor colorForMetricValue(ColorMetric metric,
                           const ColorContext& context,
                           const RideRecord& record);
bool hasColorMetricValue(ColorMetric metric, const RideRecord& record);
}
