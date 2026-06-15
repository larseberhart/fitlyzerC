#pragma once

#include <vector>

#include "analysis/ClimbMetrics.h"

struct ClimbDetectorConfig
{
    // Legacy thresholds (deprecated, use scoring instead)
    double minLengthKm = 0.15;
    double minElevationGainM = 10.0;
    double minAverageGradient = 4.0;

    // Start/continuation/end detection
    double startGradient = 1.5;
    double continueGradient = 1.0;
    double endGradient = 0.5;
    double minStartContinuousDistanceMeters = 100.0;
    double maxStartInterruptionMeters = 30.0;

    // Dip tolerance (Phase 3)
    double maxDipMeters = 10.0;
    double maxDipDistanceMeters = 200.0;
    double maxDipPctOfGain = 10.0;
    double maxSmallDipsCount = 3;  // Allow up to 3 small dips (< 5m) before terminating climb
    double smallDipThresholdMeters = 5.0;

    // Plateau detection
    double maxPlateauDistanceMeters = 500.0;
    double plateauMinGradient = -1.0;

    // Elevation smoothing (Phase 4)
    double elevationSmoothingMeters = 50.0;
    bool useAdaptiveSmoothing = true;
    double adaptiveSmoothingMinMeters = 25.0;
    double adaptiveSmoothingMaxMeters = 100.0;
    double gradientWindowHalfMeters = 25.0;

    // Spike detection
    double altitudeSpikeVerticalSpeedMps = 8.0;

    // Scoring-based detection (Phase 2)
    bool useScoringDetection = true;
    double minClimbScore = 50.0;   // composite score threshold
    double gainScoreWeight = 0.5;  // meters of gain
    double gradeScoreWeight = 4.0; // average gradient %
    double durationScoreWeight = 2.0; // duration in minutes
    double vamScoreBoost = 800.0;  // VAM threshold for score boost

    // Grade persistence (Phase 5)
    bool requireGradePersistence = true;
    double gradePersistenceThreshold = 4.0;  // 4% gradient
    double gradePersistenceMinMeters = 50.0; // must maintain grade for 50m

    // Merge fragmented climbs (Phase 7)
    bool enableClimbMerging = true;
    double mergeMaxGapMeters = 50.0;   // climbs within 50m can be merged
    double mergeMaxGapElevationM = 10.0; // or if elevation gap < 10m
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
        double windowMeters,
        const std::vector<double>& adaptiveWindowMeters = {},
        double altitudeSpikeVerticalSpeedMps = 8.0);
    static std::vector<double> computeAdaptiveSmoothingWindowsMeters(
        const RideData& rideData,
        const std::vector<double>& cumulativeDistanceMeters,
        const Config& config);
    static std::vector<double> computeLocalGradientPct(
        const std::vector<double>& smoothedAltitudeMeters,
        const std::vector<double>& cumulativeDistanceMeters,
        double halfWindowMeters);

    // Phase 2: Scoring-based detection
    static double computeClimbScore(
        double gainMeters,
        double averageGrade,
        double distanceKm,
        double continuityRatio,
        double vam);

    // Phase 6: VAM computation helper
    static double computeVAM(double elevationGainM, double durationSeconds);

    // Phase 1: Categorization
    static ClimbCategory categorizeClimb(
        double gainMeters,
        double lengthKm,
        double averageGrade);

    // Phase 7: Merge fragmented climbs
    static std::vector<Climb> mergeFragmentedClimbs(
        std::vector<Climb>& candidates,
        const Config& config);
};
