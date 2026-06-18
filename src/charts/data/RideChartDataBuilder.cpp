// SPDX-License-Identifier: GPL-3

#include "RideChartDataBuilder.h"

#include "core/zones/ColorProvider.h"

namespace RideChartDataBuilder
{
QString labelForMetric(Metric metric)
{
    switch (metric)
    {
        case Metric::Power:     return "Power (W)";
        case Metric::HeartRate: return "Heart Rate (bpm)";
        case Metric::Cadence:   return "Cadence (rpm)";
        case Metric::Speed:     return "Speed (km/h)";
        case Metric::Altitude:  return "Altitude (m)";
    }
    return {};
}

QString labelForColorMetric(ColorMetric metric)
{
    switch (metric)
    {
        case ColorMetric::Power: return "Power (W)";
        case ColorMetric::HeartRate: return "Heart Rate (bpm)";
        case ColorMetric::Cadence: return "Cadence (rpm)";
        case ColorMetric::Speed: return "Speed (km/h)";
        case ColorMetric::Altitude: return "Altitude (m)";
        case ColorMetric::Gradient: return "Gradient (%)";
        case ColorMetric::None: return "";
    }
    return "";
}

Metric metricFromColorMetric(ColorMetric metric)
{
    switch (metric)
    {
        case ColorMetric::Power: return Metric::Power;
        case ColorMetric::HeartRate: return Metric::HeartRate;
        case ColorMetric::Cadence: return Metric::Cadence;
        case ColorMetric::Speed: return Metric::Speed;
        case ColorMetric::Altitude: return Metric::Altitude;
        case ColorMetric::Gradient:
        case ColorMetric::None:
            break;
    }
    return Metric::Power;
}

double valueForMetric(const RideRecord& record, Metric metric)
{
    switch (metric)
    {
        case Metric::Power:     return record.power;
        case Metric::HeartRate: return record.heartRate;
        case Metric::Cadence:   return record.cadence;
        case Metric::Speed:     return record.speed;
        case Metric::Altitude:  return record.altitude;
    }
    return 0.0;
}

bool isValidSample(const RideRecord& record, Metric metric)
{
    switch (metric)
    {
        case Metric::Power:     return record.hasPower && record.power > 0.0;
        case Metric::HeartRate: return record.hasHeartRate;
        case Metric::Cadence:   return record.hasCadence;
        case Metric::Speed:     return record.hasSpeed;
        case Metric::Altitude:  return record.hasAltitude;
    }
    return false;
}

int autoSmoothingSecondsForSpan(double spanMinutes)
{
    if (spanMinutes < 5.0)   return 0;
    if (spanMinutes < 30.0)  return 5;
    if (spanMinutes < 120.0) return 10;
    if (spanMinutes < 360.0) return 30;
    return 60;
}

QColor colorForMetricValue(ColorMetric metric,
                           const ColorContext& context,
                           const RideRecord& record)
{
    switch (metric)
    {
        case ColorMetric::Power:
            return ColorProvider::colorForPower(record.power, context.ftp);
        case ColorMetric::HeartRate:
            return ColorProvider::colorForHeartRate(record.heartRate,
                                                    context.heartRateMax,
                                                    context.heartRateRest,
                                                    context.thresholdHeartRate);
        case ColorMetric::Cadence:
            return ColorProvider::colorForCadence(record.cadence);
        case ColorMetric::Speed:
            return ColorProvider::colorForSpeed(record.speed);
        case ColorMetric::Altitude:
            return ColorProvider::colorForAltitude(record.altitude,
                                                   context.altitudeMin,
                                                   context.altitudeMax);
        case ColorMetric::Gradient:
        case ColorMetric::None:
            return QColor("#2563eb");
    }

    return QColor("#2563eb");
}

bool hasColorMetricValue(ColorMetric metric, const RideRecord& record)
{
    switch (metric)
    {
        case ColorMetric::Power:     return record.hasPower && record.power > 0.0;
        case ColorMetric::HeartRate: return record.hasHeartRate;
        case ColorMetric::Cadence:   return record.hasCadence;
        case ColorMetric::Speed:     return record.hasSpeed;
        case ColorMetric::Altitude:  return record.hasAltitude;
        case ColorMetric::Gradient:
        case ColorMetric::None:      return false;
    }

    return false;
}
}
