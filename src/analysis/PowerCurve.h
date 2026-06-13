#pragma once

#include <vector>

#include "fit/RideData.h"

struct PowerCurvePoint
{
    double durationSeconds;
    double bestPower;        // watts — best mean for the given duration
};

class PowerCurve
{
public:
    // Standard test durations in seconds: 5 s → 2 h
    static std::vector<double> standardDurations();

    // Best mean power over a sliding window of `durationSeconds`
    static double bestMeanPower(
        const RideData& rideData, double durationSeconds);

    // PDC for standard durations (omits durations longer than the ride)
    static std::vector<PowerCurvePoint> computeStandard(
        const RideData& rideData);
};
