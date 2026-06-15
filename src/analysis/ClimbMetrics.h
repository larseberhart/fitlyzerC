#pragma once

#include <array>
#include <vector>

#include <QString>

#include "fit/RideData.h"

struct ClimbQuarterMetrics
{
    double averagePower = 0.0;
    double averageHeartRate = 0.0;
    double averageCadence = 0.0;
};

struct Climb
{
    double startSeconds = 0.0;
    double endSeconds = 0.0;

    double startDistanceKm = 0.0;
    double endDistanceKm = 0.0;

    double durationSeconds = 0.0;

    double lengthKm = 0.0;
    double elevationGainM = 0.0;

    double averageGradient = 0.0;
    double maximumGradient = 0.0;

    double averagePower = 0.0;
    double normalizedPower = 0.0;

    double averageHeartRate = 0.0;
    double averageCadence = 0.0;
    double averageSpeed = 0.0;

    double vam = 0.0;

    double powerFirstHalf = 0.0;
    double powerSecondHalf = 0.0;

    double powerFadePct = 0.0;

    double hrDriftPct = 0.0;
    double difficultyScore = 0.0;

    std::array<ClimbQuarterMetrics, 4> quarterMetrics{};

    QString name;
};

class ClimbMetrics
{
public:
    static Climb build(
        const RideData& rideData,
        const std::vector<double>& cumulativeDistanceMeters,
        const std::vector<double>& smoothedAltitudeMeters,
        const std::vector<double>& localGradientPct,
        int startIdx,
        int endIdx,
        int climbOrdinal);
};
