// SPDX-License-Identifier: GPL-3


#pragma once

#include <QString>
#include <vector>

/**
 * @brief A single recorded sample from an activity recording.
 *
 * Contains time, position, power, heart rate, and other metrics
 * recorded at one instant during an activity.
 */
struct RideRecord
{
    /// @brief Elapsed time in seconds from activity start.
    double elapsedSeconds = 0.0;

    /// @brief GPS latitude in degrees.
    double latitude = 0.0;

    /// @brief GPS longitude in degrees.
    double longitude = 0.0;

    /// @brief Power in watts.
    double power = 0.0;

    /// @brief Heart rate in bpm.
    double heartRate = 0.0;

    /// @brief Cadence in rpm.
    double cadence = 0.0;

    /// @brief Speed in km/h.
    double speed = 0.0;

    /// @brief Altitude in meters.
    double altitude = 0.0;

    /// @brief Whether GPS data is available.
    bool hasGps = false;

    /// @brief Whether power data is available.
    bool hasPower = false;

    /// @brief Whether heart rate data is available.
    bool hasHeartRate = false;

    /// @brief Whether cadence data is available.
    bool hasCadence = false;

    /// @brief Whether speed data is available.
    bool hasSpeed = false;

    /// @brief Whether altitude data is available.
    bool hasAltitude = false;
};

/**
 * @brief Container for all recorded samples and metadata for a single activity.
 */
class RideData
{
 public:
    /// @brief Recorded samples.
    std::vector<RideRecord> records;

    /// @brief Activity start time (UTC, ISO 8601).
    QString activityStartTimeUtc;

    /// @brief Activity end time (UTC, ISO 8601).
    QString activityEndTimeUtc;

    /// @brief File timestamp (UTC, ISO 8601).
    QString fileTimestampUtc;
};