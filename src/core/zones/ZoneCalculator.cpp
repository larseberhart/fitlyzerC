#include "ZoneCalculator.h"

#include <algorithm>
#include <cfloat>

namespace {

double valueForMetric(const RideRecord& record, ColorMetric metric)
{
    switch (metric)
    {
        case ColorMetric::Power:     return record.power;
        case ColorMetric::HeartRate: return record.heartRate;
        case ColorMetric::Cadence:   return record.cadence;
        case ColorMetric::Speed:     return record.speed;
        case ColorMetric::Altitude:  return record.altitude;
        case ColorMetric::Gradient:  return 0.0;
        case ColorMetric::None:      return 0.0;
    }

    return 0.0;
}

bool sampleAvailable(const RideRecord& record, ColorMetric metric)
{
    switch (metric)
    {
        case ColorMetric::Power:     return record.hasPower;
        case ColorMetric::HeartRate: return record.hasHeartRate;
        case ColorMetric::Cadence:   return record.hasCadence;
        case ColorMetric::Speed:     return record.hasSpeed;
        case ColorMetric::Altitude:  return record.hasAltitude;
        case ColorMetric::Gradient:  return false;
        case ColorMetric::None:      return false;
    }

    return false;
}

double sampleDurationSeconds(const std::vector<RideRecord>& records, int index)
{
    const int size = static_cast<int>(records.size());
    if (size <= 1)
        return 1.0;

    double dt = 1.0;
    if (index == 0)
        dt = records[1].elapsedSeconds - records[0].elapsedSeconds;
    else if (index == size - 1)
        dt = records[size - 1].elapsedSeconds - records[size - 2].elapsedSeconds;
    else
        dt = (records[index + 1].elapsedSeconds - records[index - 1].elapsedSeconds) / 2.0;

    return std::max(0.0, dt);
}

}

std::vector<Zone> ZoneCalculator::powerZones(int ftp)
{
    const double threshold = std::max(ftp, 1);
    return {
        {"Z1 Recovery",    0.0,             threshold * 0.55, QColor("#b0b0b0")},
        {"Z2 Endurance",   threshold * 0.55, threshold * 0.75, QColor("#6fa8dc")},
        {"Z3 Tempo",       threshold * 0.75, threshold * 0.87, QColor("#6aa84f")},
        {"Z4 Threshold",   threshold * 0.87, threshold * 0.95, QColor("#ffd966")},
        {"Z5 VO2 Max",     threshold * 0.95, threshold * 1.05, QColor("#e69138")},
        {"Z6 Anaerobic",   threshold * 1.05, threshold * 1.20, QColor("#cc0000")},
        {"Z7 Neuromuscular", threshold * 1.20, DBL_MAX,        QColor("#741b47")},
    };
}

std::vector<Zone> ZoneCalculator::heartRateZones(int heartRateMax,
                                                  int,
                                                  int thresholdHeartRate)
{
    const double reference = std::max(thresholdHeartRate > 0 ? thresholdHeartRate : heartRateMax, 120);
    return {
        {"Z1 Recovery",  0.0,            reference * 0.68, QColor("#b0b0b0")},
        {"Z2 Aerobic",   reference * 0.68, reference * 0.83, QColor("#6fa8dc")},
        {"Z3 Tempo",     reference * 0.83, reference * 0.94, QColor("#6aa84f")},
        {"Z4 Threshold", reference * 0.94, reference * 1.03, QColor("#ffd966")},
        {"Z5 Max",       reference * 1.03, DBL_MAX,          QColor("#cc0000")},
    };
}

std::vector<Zone> ZoneCalculator::cadenceZones()
{
    return {
        {"Low",        0.0,  70.0, QColor("#b0b0b0")},
        {"Steady",    70.0,  85.0, QColor("#6fa8dc")},
        {"Endurance", 85.0,  95.0, QColor("#6aa84f")},
        {"Fast",      95.0, 105.0, QColor("#ffd966")},
        {"Spin",     105.0, DBL_MAX, QColor("#cc0000")},
    };
}

std::vector<Zone> ZoneCalculator::speedZones()
{
    return {
        {"Easy",      0.0,  15.0, QColor("#6fa8dc")},
        {"Rolling",  15.0,  25.0, QColor("#6aa84f")},
        {"Tempo",    25.0,  35.0, QColor("#ffd966")},
        {"Fast",     35.0,  45.0, QColor("#e69138")},
        {"Descent",  45.0, DBL_MAX, QColor("#cc0000")},
    };
}

std::vector<Zone> ZoneCalculator::altitudeZones(double minAltitude,
                                                 double maxAltitude)
{
    if (maxAltitude <= minAltitude)
        maxAltitude = minAltitude + 1.0;

    const double span = maxAltitude - minAltitude;
    return {
        {"Low",       minAltitude,                minAltitude + span * 0.25, QColor("#6fa8dc")},
        {"Rolling",   minAltitude + span * 0.25, minAltitude + span * 0.55, QColor("#6aa84f")},
        {"High",      minAltitude + span * 0.55, minAltitude + span * 0.80, QColor("#8d6e63")},
        {"Alpine",    minAltitude + span * 0.80, DBL_MAX,                   QColor("#f5f5f5")},
    };
}

std::vector<Zone> ZoneCalculator::zonesForMetric(ColorMetric metric,
                                                  const ColorContext& context)
{
    switch (metric)
    {
        case ColorMetric::Power:
            return powerZones(context.ftp);
        case ColorMetric::HeartRate:
            return heartRateZones(context.heartRateMax,
                                  context.heartRateRest,
                                  context.thresholdHeartRate);
        case ColorMetric::Cadence:
            return cadenceZones();
        case ColorMetric::Speed:
            return speedZones();
        case ColorMetric::Altitude:
            return altitudeZones(context.altitudeMin, context.altitudeMax);
        case ColorMetric::Gradient:
        case ColorMetric::None:
            return {};
    }

    return {};
}

std::vector<ZoneDistribution> ZoneCalculator::computeDistribution(
    const RideData& rideData,
    ColorMetric metric,
    const ColorContext& context)
{
    std::vector<ZoneDistribution> result;
    const std::vector<Zone> zones = zonesForMetric(metric, context);
    result.reserve(zones.size());
    for (const Zone& zone : zones)
        result.push_back({zone, 0.0, 0.0});

    if (metric == ColorMetric::None || rideData.records.empty() || result.empty())
        return result;

    if (metric == ColorMetric::Gradient)
        return result;
    double totalSeconds = 0.0;
    for (int index = 0; index < static_cast<int>(rideData.records.size()); ++index)
    {
        const RideRecord& record = rideData.records[static_cast<size_t>(index)];
        if (!sampleAvailable(record, metric))
            continue;

        const double value = valueForMetric(record, metric);
        const double dt = sampleDurationSeconds(rideData.records, index);
        totalSeconds += dt;

        for (ZoneDistribution& distribution : result)
        {
            if (value >= distribution.zone.minValue &&
                value < distribution.zone.maxValue)
            {
                distribution.seconds += dt;
                break;
            }
        }
    }

    if (totalSeconds > 0.0)
    {
        for (ZoneDistribution& distribution : result)
            distribution.percent = distribution.seconds / totalSeconds;
    }

    return result;
}

bool ZoneCalculator::hasMetricSamples(const RideData& rideData, ColorMetric metric)
{
    if (metric == ColorMetric::None)
        return false;

    for (const RideRecord& record : rideData.records)
    {
        if (sampleAvailable(record, metric))
            return true;
    }

    return false;
}