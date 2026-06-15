#pragma once

#include <vector>

#include "analysis/ClimbMetrics.h"

struct ClimbDetectorConfig
{
    double minLengthKm = 0.5;
    double minElevationGainM = 30.0;
    double minAverageGradient = 3.0;

    double startGradient = 3.0;
    double minStartContinuousDistanceMeters = 100.0;

    double maxDipMeters = 10.0;
    double maxDipDistanceMeters = 200.0;

    double elevationSmoothingMeters = 50.0;
    double gradientWindowHalfMeters = 25.0;
};

class ClimbDetector
{
public:
    using Config = ClimbDetectorConfig;

    static std::vector<Climb> detect(
        const RideData& rideData,
        const Config& config = Config{});

    static std::vector<double> buildCumulativeDistanceMeters(const RideData& rideData);
    static std::vector<double> smoothAltitudeByDistance(
        const RideData& rideData,
        const std::vector<double>& cumulativeDistanceMeters,
        double windowMeters);
    static std::vector<double> computeLocalGradientPct(
        const std::vector<double>& smoothedAltitudeMeters,
        const std::vector<double>& cumulativeDistanceMeters,
        double halfWindowMeters);
};
