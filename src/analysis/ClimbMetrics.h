// SPDX-License-Identifier: GPL-3

/**
 * @file ClimbMetrics.h
 * @brief Analysis component for ClimbMetrics.
 *
 * Implements analysis logic used to compute cycling metrics, detect patterns, and derive activity insights.
 *
 * Responsibilities:
 * - Provide analysis-specific functionality for activity processing
 *
 * @author Lars EBERHART
 */

#pragma once

#include <array>
#include <vector>

#include <QString>

#include "fit/RideData.h"

/**
 * @brief Classification of detected climbs by difficulty.
 */
enum class ClimbCategory
{
    Major,       // >100m gain, >2km length, >3% avg grade
    Medium,      // >40m gain, >500m length, >3% avg grade
    Punchy,      // >10m gain, >100m length, >5% avg grade (short steep)
    FalsePositive // failed quality checks
};

/**
 * @brief Metrics for a quarter of a climb segment.
 */
struct ClimbQuarterMetrics
{
    /// @brief Average power during quarter.
    double averagePower = 0.0;

    /// @brief Average heart rate during quarter.
    double averageHeartRate = 0.0;

    /// @brief Average cadence during quarter.
    double averageCadence = 0.0;
};

/**
 * @brief Represents a detected climb with comprehensive metrics.
 *
 * Contains temporal, spatial, effort, and performance data for a single detected climb.
 */
struct Climb
{
    /// @brief Unique climb identifier (database row id).
    int    id = -1;

    /// @brief Climb start time in seconds from activity start.
    double startSeconds = 0.0;

    /// @brief Climb end time in seconds from activity start.
    double endSeconds = 0.0;

    /// @brief Climb start distance in km from activity start.
    double startDistanceKm = 0.0;

    /// @brief Climb end distance in km from activity start.
    double endDistanceKm = 0.0;

    /// @brief Climb duration in seconds.
    double durationSeconds = 0.0;

    /// @brief Climb length in km.
    double lengthKm = 0.0;

    /// @brief Total elevation gain in meters.
    double elevationGainM = 0.0;

    /// @brief Starting elevation in meters.
    double startElevationM = 0.0;

    /// @brief Ending elevation in meters.
    double endElevationM = 0.0;

    /// @brief Average gradient percentage.
    double averageGradient = 0.0;

    /// @brief Maximum gradient percentage.
    double maximumGradient = 0.0;

    /// @brief Composite climb score.
    double climbScore = 0.0;

    /// @brief Percentile of climb with grade above threshold.
    double gradePercentile = 0.0;

    /// @brief Climb category (Major, Medium, Punchy, FalsePositive).
    ClimbCategory climbCategory = ClimbCategory::FalsePositive;

    /// @brief Average power in watts.
    double averagePower = 0.0;

    /// @brief Average power to weight in watts/kg.
    double averageWattsPerKg = 0.0;

    /// @brief Normalized power in watts.
    double normalizedPower = 0.0;

    /// @brief Average heart rate in bpm.
    double averageHeartRate = 0.0;

    /// @brief Average cadence in rpm.
    double averageCadence = 0.0;

    /// @brief Average speed in km/h.
    double averageSpeed = 0.0;

    /// @brief VAM (Vertical Ascent Meter) in m/h.
    double vam = 0.0;

    /// @brief Average power in first half.
    double powerFirstHalf = 0.0;

    /// @brief Average power in second half.
    double powerSecondHalf = 0.0;

    /// @brief Power fade percentage from first to second half.
    double powerFadePct = 0.0;

    /// @brief Aerobic decoupling percentage.
    double aerobicDecouplingPct = 0.0;

    /// @brief Heart rate drift percentage.
    double hrDriftPct = 0.0;

    /// @brief Overall difficulty score.
    double difficultyScore = 0.0;

    /// @brief Metrics for each quarter of climb.
    std::array<ClimbQuarterMetrics, 4> quarterMetrics{};

    /// @brief Category description string.
    QString category;

    /// @brief Shape classification (steep, ramp, var, etc.).
    QString shapeClass;

    /// @brief Watts/kg performance ranking.
    QString wattsPerKgRank;

    /// @brief Climb name.
    QString name;
};

/**
 * @brief Computes comprehensive climb metrics from activity data.
 */
class ClimbMetrics
{
public:
    /**
     * @brief Builds a complete Climb object from activity data.
     * @param rideData Activity recording data.
     * @param cumulativeDistanceMeters Distance array.
     * @param smoothedAltitudeMeters Elevation array (smoothed).
     * @param localGradientPct Gradient array.
     * @param startIdx Index of climb start.
     * @param endIdx Index of climb end.
     * @param climbOrdinal Ordinal position of climb in activity.
     * @return Fully populated Climb structure with all metrics.
     */
    static Climb build(
        const RideData& rideData,
        const std::vector<double>& cumulativeDistanceMeters,
        const std::vector<double>& smoothedAltitudeMeters,
        const std::vector<double>& localGradientPct,
        int startIdx,
        int endIdx,
        int climbOrdinal);
};