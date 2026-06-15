// SPDX-License-Identifier: GPL-3

/**
 * @file ClimbDetector.h
 * @brief Analysis component for ClimbDetector.
 *
 * Implements analysis logic used to compute cycling metrics, detect patterns, and derive activity insights.
 *
 * Responsibilities:
 * - Provide analysis-specific functionality for activity processing
 *
 * @author Lars EBERHART
 */

#pragma once

#include <vector>

#include "analysis/ClimbMetrics.h"

/**
 * @brief Configuration parameters for climb detection.
 *
 * Contains tunable thresholds and flags used to control climb detection behavior,
 * including gradient thresholds, distance tolerances, and elevation smoothing parameters.
 */
struct ClimbDetectorConfig
{
    /// @brief Minimum climb length in km (legacy).
    double minLengthKm = 0.15;
    /// @brief Minimum elevation gain in meters (legacy).
    double minElevationGainM = 10.0;
    /// @brief Minimum average gradient (legacy).
    double minAverageGradient = 4.0;

    /// @brief Gradient threshold to start climb detection (%).
    double startGradient = 1.5;
    /// @brief Gradient threshold to continue climb (%).
    double continueGradient = 1.0;
    /// @brief Gradient threshold to end climb (%).
    double endGradient = 0.5;
    /// @brief Minimum continuous distance at start gradient (meters).
    double minStartContinuousDistanceMeters = 100.0;
    /// @brief Maximum interruption when starting climb (meters).
    double maxStartInterruptionMeters = 30.0;

    /// @brief Maximum dip elevation loss (meters).
    double maxDipMeters = 10.0;
    /// @brief Maximum dip horizontal distance (meters).
    double maxDipDistanceMeters = 200.0;
    /// @brief Maximum dip as percentage of gain (%).
    double maxDipPctOfGain = 10.0;
    /// @brief Maximum number of small dips (< 5m) allowed.
    double maxSmallDipsCount = 3;
    /// @brief Threshold for small dip detection (meters).
    double smallDipThresholdMeters = 5.0;

    /// @brief Maximum distance for plateau detection (meters).
    double maxPlateauDistanceMeters = 500.0;
    /// @brief Minimum gradient for plateau (%).
    double plateauMinGradient = -1.0;

    /// @brief Elevation smoothing window (meters).
    double elevationSmoothingMeters = 50.0;
    /// @brief Enable adaptive smoothing based on grade changes.
    bool useAdaptiveSmoothing = true;
    /// @brief Minimum adaptive smoothing window (meters).
    double adaptiveSmoothingMinMeters = 25.0;
    /// @brief Maximum adaptive smoothing window (meters).
    double adaptiveSmoothingMaxMeters = 100.0;
    /// @brief Half-window for gradient computation (meters).
    double gradientWindowHalfMeters = 25.0;

    /// @brief Altitude spike detection threshold (m/s).
    double altitudeSpikeVerticalSpeedMps = 8.0;

    /// @brief Enable scoring-based climb detection.
    bool useScoringDetection = true;
    /// @brief Minimum composite climb score.
    double minClimbScore = 50.0;
    /// @brief Weight for elevation gain in score.
    double gainScoreWeight = 0.5;
    /// @brief Weight for average grade in score.
    double gradeScoreWeight = 4.0;
    /// @brief Weight for duration in score.
    double durationScoreWeight = 2.0;
    /// @brief VAM threshold for score boost.
    double vamScoreBoost = 800.0;

    /// @brief Enable grade persistence requirement.
    bool requireGradePersistence = true;
    /// @brief Minimum gradient for persistence (%).
    double gradePersistenceThreshold = 4.0;
    /// @brief Minimum distance at grade for persistence (meters).
    double gradePersistenceMinMeters = 50.0;

    /// @brief Enable merge of fragmented climbs.
    bool enableClimbMerging = true;
    /// @brief Maximum gap between climbs to merge (meters).
    double mergeMaxGapMeters = 50.0;
    /// @brief Maximum elevation gap to allow merge (meters).
    double mergeMaxGapElevationM = 10.0;
};

/**
 * @brief Detects and categorizes climbs in cycling activities.
 *
 * Provides static methods for analyzing activity elevation data to identify climbs
 * using multi-phase detection including gradient thresholds, scoring, and merge logic.
 */
class ClimbDetector
{
public:
    /// @brief Configuration typedef for climb detection parameters.
    using Config = ClimbDetectorConfig;

    /**
     * @brief Detects climbs in activity data using multi-phase algorithm.
     * @param rideData Activity recording data.
     * @param config Detection configuration parameters.
     * @return Vector of detected climbs.
     */
    static std::vector<Climb> detect(
        const RideData& rideData,
        const Config& config = Config{});

    /**
     * @brief Builds cumulative distance array for activity.
     * @param rideData Activity data.
     * @return Array of cumulative distances in meters.
     */
    static std::vector<double> buildCumulativeDistanceMeters(const RideData& rideData);

    /**
     * @brief Smooths altitude data using distance-based rolling window.
     * @param rideData Activity data.
     * @param cumulativeDistanceMeters Cumulative distances.
     * @param windowMeters Fixed smoothing window in meters.
     * @param adaptiveWindowMeters Optional adaptive window per sample.
     * @param altitudeSpikeVerticalSpeedMps Spike detection threshold in m/s.
     * @return Smoothed altitude array.
     */
    static std::vector<double> smoothAltitudeByDistance(
        const RideData& rideData,
        const std::vector<double>& cumulativeDistanceMeters,
        double windowMeters,
        const std::vector<double>& adaptiveWindowMeters = {},
        double altitudeSpikeVerticalSpeedMps = 8.0);

    /**
     * @brief Computes adaptive smoothing windows for elevation data.
     * @param rideData Activity data.
     * @param cumulativeDistanceMeters Cumulative distances.
     * @param config Configuration with adaptive smoothing parameters.
     * @return Array of adaptive window sizes.
     */
    static std::vector<double> computeAdaptiveSmoothingWindowsMeters(
        const RideData& rideData,
        const std::vector<double>& cumulativeDistanceMeters,
        const Config& config);

    /**
     * @brief Computes local gradient at each point.
     * @param smoothedAltitudeMeters Smoothed elevation data.
     * @param cumulativeDistanceMeters Cumulative distances.
     * @param halfWindowMeters Half-width of gradient window in meters.
     * @return Array of gradient percentages.
     */
    static std::vector<double> computeLocalGradientPct(
        const std::vector<double>& smoothedAltitudeMeters,
        const std::vector<double>& cumulativeDistanceMeters,
        double halfWindowMeters);

    /**
     * @brief Computes scoring-based climb difficulty metric.
     * @param gainMeters Elevation gain in meters.
     * @param averageGrade Average gradient percentage.
     * @param distanceKm Climb distance in kilometers.
     * @param continuityRatio Ratio of continuous vs interrupted climbing.
     * @param vam Vertical ascent speed in meters/hour.
     * @return Computed climb score.
     */
    static double computeClimbScore(
        double gainMeters,
        double averageGrade,
        double distanceKm,
        double continuityRatio,
        double vam);

    /**
     * @brief Computes vertical ascent metric (VAM).
     * @param elevationGainM Elevation gain in meters.
     * @param durationSeconds Duration in seconds.
     * @return VAM in meters/hour.
     */
    static double computeVAM(double elevationGainM, double durationSeconds);

    /**
     * @brief Categorizes a climb by difficulty.
     * @param gainMeters Elevation gain in meters.
     * @param lengthKm Climb length in kilometers.
     * @param averageGrade Average gradient percentage.
     * @return Category classification.
     */
    static ClimbCategory categorizeClimb(
        double gainMeters,
        double lengthKm,
        double averageGrade);

    /**
     * @brief Merges fragmented climb candidates into continuous climbs.
     * @param candidates Vector of initial climb candidates.
     * @param config Merge configuration parameters.
     * @return Merged climb results.
     */
    static std::vector<Climb> mergeFragmentedClimbs(
        std::vector<Climb>& candidates,
        const Config& config);
};