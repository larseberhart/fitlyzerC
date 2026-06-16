// SPDX-License-Identifier: GPL-3


#pragma once

#include "fit/RideData.h"

/**
 * @brief Statistics summary for a workout activity.
 */
struct WorkoutStatistics
{
    /// @brief Total elapsed time in seconds.
    double durationSeconds   = 0.0;

    /// @brief Moving time in seconds (speed > 2 km/h).
    double movingTimeSeconds = 0.0;

    /// @brief Total distance in kilometers.
    double totalDistanceKm   = 0.0;

    /// @brief Total elevation gain in meters.
    double elevationGainM    = 0.0;

    /// @brief Average power in watts.
    double averagePower      = 0.0;

    /// @brief Maximum power in watts.
    double maximumPower      = 0.0;

    /// @brief Average heart rate in bpm.
    double averageHeartRate  = 0.0;

    /// @brief Maximum heart rate in bpm.
    double maximumHeartRate  = 0.0;

    /// @brief Average cadence in rpm.
    double averageCadence    = 0.0;

    /// @brief Maximum cadence in rpm.
    double maximumCadence    = 0.0;

    /// @brief Average speed in km/h.
    double averageSpeed      = 0.0;

    /// @brief Maximum speed in km/h.
    double maximumSpeed      = 0.0;
};

/**
 * @brief Analyzes ride data to compute workout statistics.
 */
class WorkoutAnalyzer
{
public:
    /**
     * @brief Analyzes activity data to compute statistics.
     * @param rideData Activity recording data.
     * @return Computed statistics.
     */
    WorkoutStatistics analyze(const RideData& rideData);
};