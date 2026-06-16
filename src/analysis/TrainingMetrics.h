// SPDX-License-Identifier: GPL-3


#pragma once

#include "fit/RideData.h"

/**
 * @brief Summary of training-related metrics for an activity.
 */
struct TrainingMetricsSummary
{
    /// @brief Normalized power in watts.
    double normalizedPower = 0.0;

    /// @brief Intensity factor (normalized power / FTP).
    double intensityFactor = 0.0;

    /// @brief Training stress score (BikeScore).
    double trainingStressScore = 0.0;

    /// @brief Variability index (normalized power / average power).
    double variabilityIndex = 0.0;

    /// @brief Efficiency factor (average power / average heart rate).
    double efficiencyFactor = 0.0;

    /// @brief Average power in watts.
    double averagePower = 0.0;

    /// @brief Average heart rate in bpm.
    double averageHeartRate = 0.0;
};

/**
 * @brief Computes training metrics from activity data.
 */
class TrainingMetrics
{
public:
    /**
     * @brief Computes training metrics for an activity.
     * @param rideData Activity data.
     * @param ftp Functional threshold power in watts.
     * @param durationSeconds Optional duration override (0 = use data duration).
     * @return Metrics summary.
     */
    static TrainingMetricsSummary compute(const RideData& rideData,
                                          double ftp,
                                          double durationSeconds = 0.0);
};