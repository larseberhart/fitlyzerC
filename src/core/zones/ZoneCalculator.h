#pragma once

#include <vector>

#include "ZoneDefinition.h"
#include "fit/RideData.h"

class ZoneCalculator
{
public:
    static std::vector<Zone> powerZones(int ftp);
    static std::vector<Zone> heartRateZones(int heartRateMax,
                                            int heartRateRest = 60,
                                            int thresholdHeartRate = 0);
    static std::vector<Zone> cadenceZones();
    static std::vector<Zone> speedZones();
    static std::vector<Zone> altitudeZones(double minAltitude,
                                           double maxAltitude);

    static std::vector<Zone> zonesForMetric(ColorMetric metric,
                                            const ColorContext& context);
    static std::vector<ZoneDistribution> computeDistribution(
        const RideData& rideData,
        ColorMetric metric,
        const ColorContext& context);

    static bool hasMetricSamples(const RideData& rideData, ColorMetric metric);
};