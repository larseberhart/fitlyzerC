// SPDX-License-Identifier: GPL-3


#pragma once

#include <string>
#include <vector>

#include "fit/RideData.h"

/**
 * @brief Represents a single work or recovery interval.
 */
struct Interval
{
    /// @brief Interval start time in seconds from activity start.
    double startSeconds     = 0.0;

    /// @brief Interval end time in seconds from activity start.
    double endSeconds       = 0.0;

    /// @brief Interval duration in seconds.
    double durationSeconds  = 0.0;

    /// @brief Average power in watts.
    double averagePower     = 0.0;

    /// @brief Maximum power in watts.
    double maximumPower     = 0.0;

    /// @brief Normalized power in watts.
    double normalizedPower  = 0.0;

    /// @brief Average heart rate in bpm.
    double averageHeartRate = 0.0;

    /// @brief Average cadence in rpm.
    double averageCadence   = 0.0;

    /// @brief Average speed in km/h.
    double averageSpeed     = 0.0;

    /// @brief Interval type label (Work or Recovery).
    std::string label;
};

/**
 * @brief Configuration parameters for interval detection.
 */
struct IntervalDetectorConfig
{
    /// @brief Functional threshold power in watts.
    double ftp                = 250.0;

    /// @brief Work threshold as fraction of FTP.
    double workThresholdPct   = 0.76;

    /// @brief Minimum work interval duration in seconds.
    double minWorkSeconds     = 60.0;

    /// @brief Minimum recovery interval duration in seconds.
    double minRecoverySeconds = 20.0;

    /// @brief Rolling average window width in samples.
    int    smoothingWindow    = 30;
};

/**
 * @brief Detects work and recovery intervals in activity data.
 *
 * Identifies distinct work intervals (above power threshold) and recovery
 * periods based on configurable FTP-derived thresholds.
 */
class IntervalDetector
{
public:
    /// @brief Interval detection configuration structure.
    using Config = IntervalDetectorConfig;

    /**
     * @brief Detects intervals in activity data.
     * @param rideData Activity recording data.
     * @param config Detection parameters.
     * @return List of detected intervals.
     */
    static std::vector<Interval> detect(
        const RideData& rideData,
        const Config& config = Config{});

    /**
     * @brief Removes duplicate and overlapping intervals.
     * @param intervals List of intervals to clean.
     * @return Cleaned list (within 1 second tolerance).
     */
    static std::vector<Interval> removeOverlappingAndDuplicates(
        std::vector<Interval> intervals);

private:
    static std::vector<double> smoothPower(
        const RideData& rideData, int window);

    static Interval buildInterval(
        const RideData& rideData,
        int startIdx, int endIdx,
        const std::string& label);
};