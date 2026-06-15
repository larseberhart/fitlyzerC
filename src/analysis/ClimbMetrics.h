#pragma once

#include <array>
#include <vector>

#include <QString>

#include "fit/RideData.h"

enum class ClimbCategory
{
    Major,       // >100m gain, >2km length, >3% avg grade
    Medium,      // >40m gain, >500m length, >3% avg grade
    Punchy,      // >10m gain, >100m length, >5% avg grade (short steep)
    FalsePositive // failed quality checks
};

struct ClimbQuarterMetrics
{
    double averagePower = 0.0;
    double averageHeartRate = 0.0;
    double averageCadence = 0.0;
};

struct Climb
{
    int    id = -1; // database row id (-1 = not yet persisted)

    double startSeconds = 0.0;
    double endSeconds = 0.0;

    double startDistanceKm = 0.0;
    double endDistanceKm = 0.0;

    double durationSeconds = 0.0;

    double lengthKm = 0.0;
    double elevationGainM = 0.0;
    double startElevationM = 0.0;
    double endElevationM = 0.0;

    double averageGradient = 0.0;
    double maximumGradient = 0.0;

    double climbScore = 0.0;       // composite scoring (gain + grade + duration + consistency)
    double gradePercentile = 0.0;  // percentage of climb with grade > threshold
    ClimbCategory climbCategory = ClimbCategory::FalsePositive;

    double averagePower = 0.0;
    double averageWattsPerKg = 0.0;
    double normalizedPower = 0.0;

    double averageHeartRate = 0.0;
    double averageCadence = 0.0;
    double averageSpeed = 0.0;

    double vam = 0.0;

    double powerFirstHalf = 0.0;
    double powerSecondHalf = 0.0;

    double powerFadePct = 0.0;

    double aerobicDecouplingPct = 0.0;
    double hrDriftPct = 0.0;
    double difficultyScore = 0.0;

    std::array<ClimbQuarterMetrics, 4> quarterMetrics{};

    QString category;
    QString shapeClass;
    QString wattsPerKgRank;
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
