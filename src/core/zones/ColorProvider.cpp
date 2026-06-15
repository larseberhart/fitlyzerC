// SPDX-License-Identifier: GPL-3

/**
 * @file ColorProvider.cpp
 * @brief Metric-based color generation for charts and maps.
 *
 * Converts activity metrics such as power, heart rate,
 * cadence, speed, altitude, and gradient into display
 * colors for visualization components.
 *
 * @author Lars EBERHART
 */

#include "ColorProvider.h"

#include "ZoneCalculator.h"

#include <algorithm>

namespace {

QColor interpolateColor(const QColor& a, const QColor& b, double t)
{
    const double clamped = std::clamp(t, 0.0, 1.0);
    return QColor(
        static_cast<int>(a.red()   + (b.red()   - a.red())   * clamped),
        static_cast<int>(a.green() + (b.green() - a.green()) * clamped),
        static_cast<int>(a.blue()  + (b.blue()  - a.blue())  * clamped));
}

}

QColor ColorProvider::colorForMetric(ColorMetric metric,
                                     double value,
                                     const ColorContext& context)
{
    switch (metric)
    {
        case ColorMetric::Power:
            return colorForPower(value, context.ftp);
        case ColorMetric::HeartRate:
            return colorForHeartRate(value,
                                     context.heartRateMax,
                                     context.heartRateRest,
                                     context.thresholdHeartRate);
        case ColorMetric::Cadence:
            return colorForCadence(value);
        case ColorMetric::Speed:
            return colorForSpeed(value);
        case ColorMetric::Altitude:
            return colorForAltitude(value, context.altitudeMin, context.altitudeMax);
        case ColorMetric::Gradient:
        case ColorMetric::None:
            return QColor("#94a3b8");
    }

    return QColor("#94a3b8");
}

QColor ColorProvider::colorForPower(double watts, int ftp)
{
    return colorForValue(watts, ZoneCalculator::powerZones(ftp));
}

QColor ColorProvider::colorForHeartRate(double bpm,
                                        int heartRateMax,
                                        int heartRateRest,
                                        int thresholdHeartRate)
{
    return colorForValue(
        bpm,
        ZoneCalculator::heartRateZones(heartRateMax, heartRateRest, thresholdHeartRate));
}

QColor ColorProvider::colorForCadence(double cadence)
{
    return colorForValue(cadence, ZoneCalculator::cadenceZones());
}

QColor ColorProvider::colorForSpeed(double speedKmh)
{
    return colorForValue(speedKmh, ZoneCalculator::speedZones());
}

QColor ColorProvider::colorForAltitude(double altitude,
                                       double minAltitude,
                                       double maxAltitude)
{
    if (maxAltitude <= minAltitude)
        return QColor("#6fa8dc");

    const double t = std::clamp((altitude - minAltitude) / (maxAltitude - minAltitude), 0.0, 1.0);
    if (t < 0.33)
        return interpolateColor(QColor("#6fa8dc"), QColor("#6aa84f"), t / 0.33);
    if (t < 0.66)
        return interpolateColor(QColor("#6aa84f"), QColor("#8d6e63"), (t - 0.33) / 0.33);
    return interpolateColor(QColor("#8d6e63"), QColor("#f5f5f5"), (t - 0.66) / 0.34);
}

QColor ColorProvider::colorForValue(double value,
                                    const std::vector<Zone>& zones,
                                    const QColor& fallback)
{
    for (const Zone& zone : zones)
    {
        if (value >= zone.minValue && value < zone.maxValue)
            return zone.color;
    }

    return fallback;
}