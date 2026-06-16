// SPDX-License-Identifier: GPL-3


#pragma once

#include <vector>

#include "fit/RideData.h"

/**
 * @brief A single point on a power duration curve.
 */
struct PowerCurvePoint
{
    /// @brief Duration in seconds.
    double durationSeconds;

    /// @brief Best mean power for duration in watts.
    double bestPower;
};

/**
 * @brief Computes power duration curve (PDC) for an activity.
 *
 * Analyzes best sustained power outputs across a range of durations.
 */
class PowerCurve
{
public:
    /**
     * @brief Returns standard test durations in seconds.
     * @return Durations from 5 seconds to 2 hours.
     */
    static std::vector<double> standardDurations();

    /**
     * @brief Calculates best mean power over a duration.
     * @param rideData Activity data.
     * @param durationSeconds Duration window in seconds.
     * @return Best mean power in watts.
     */
    static double bestMeanPower(
        const RideData& rideData, double durationSeconds);

    /**
     * @brief Computes power duration curve for standard durations.
     * @param rideData Activity data.
     * @return PDC points (omits durations longer than the ride).
     */
    static std::vector<PowerCurvePoint> computeStandard(
        const RideData& rideData);
};