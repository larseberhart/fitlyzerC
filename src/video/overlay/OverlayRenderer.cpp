// SPDX-License-Identifier: GPL-3

#include "OverlayRenderer.h"

#include <cmath>
#include <limits>

namespace OverlayRenderer
{
QString formatDuration(double seconds)
{
    const int total = static_cast<int>(std::round(std::max(0.0, seconds)));
    const int h = total / 3600;
    const int m = (total % 3600) / 60;
    const int s = total % 60;
    if (h > 0)
        return QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
    return QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
}

QString formatLegendValue(double value, ColorMetric metric)
{
    switch (metric)
    {
        case ColorMetric::Speed:
            return QString::number(value, 'f', 1);
        case ColorMetric::Power:
        case ColorMetric::HeartRate:
        case ColorMetric::Cadence:
        case ColorMetric::Altitude:
            return QString::number(static_cast<int>(std::round(value)));
        case ColorMetric::Gradient:
        case ColorMetric::None:
            return QString::number(value, 'f', 1);
    }

    return QString::number(value, 'f', 1);
}

QString zoneLegendText(const Zone& zone, ColorMetric metric)
{
    const QString unit = colorMetricUnit(metric);
    const bool hasUnit = !unit.isEmpty();
    const QString valueSuffix = hasUnit ? QString(" %1").arg(unit) : QString();
    const double hugeThreshold = std::numeric_limits<double>::max() * 0.5;
    const bool openEnded = !std::isfinite(zone.maxValue) || zone.maxValue >= hugeThreshold;

    if (openEnded)
        return QString("%1 > %2%3").arg(zone.name, formatLegendValue(zone.minValue, metric), valueSuffix);
    if (zone.minValue <= 0.0)
        return QString("%1 < %2%3").arg(zone.name, formatLegendValue(zone.maxValue, metric), valueSuffix);
    return QString("%1 %2-%3%4")
        .arg(zone.name,
             formatLegendValue(zone.minValue, metric),
             formatLegendValue(zone.maxValue, metric),
             valueSuffix);
}
}
