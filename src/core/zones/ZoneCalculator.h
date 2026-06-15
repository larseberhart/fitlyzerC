// SPDX-License-Identifier: GPL-3

/**
 * @file ZoneCalculator.h
 * @brief Training zone support for ZoneCalculator.
 *
 * Implements training-zone definitions, calculations, and related visual behavior for zone-based analysis.
 *
 * Responsibilities:
 * - Provide zone definitions, calculations, or zone-related utilities
 *
 * @author Lars EBERHART
 */

#pragma once

#include <vector>

#include "ZoneDefinition.h"
#include "fit/RideData.h"

/**
 * @brief Calculates training zones for various metrics.
 *
 * Provides zone definitions and distributions for power, heart rate, cadence, speed, and altitude.
 */
class ZoneCalculator
{
public:
    /**
     * @brief Computes power zones based on FTP.
     * @param ftp Functional threshold power in watts.
     * @return List of power zones.
     */
    static std::vector<Zone> powerZones(int ftp);

    /**
     * @brief Computes heart rate zones.
     * @param heartRateMax Maximum heart rate.
     * @param heartRateRest Resting heart rate (default 60).
     * @param thresholdHeartRate Threshold HR (default 0).
     * @return List of heart rate zones.
     */
    static std::vector<Zone> heartRateZones(int heartRateMax,
                                            int heartRateRest = 60,
                                            int thresholdHeartRate = 0);

    /**
     * @brief Computes cadence zones.
     * @return List of cadence zones.
     */
    static std::vector<Zone> cadenceZones();

    /**
     * @brief Computes speed zones.
     * @return List of speed zones.
     */
    static std::vector<Zone> speedZones();

    /**
     * @brief Computes altitude zones.
     * @param minAltitude Minimum altitude in range.
     * @param maxAltitude Maximum altitude in range.
     * @return List of altitude zones.
     */
    static std::vector<Zone> altitudeZones(double minAltitude,
                                           double maxAltitude);

    /**
     * @brief Gets zones for a specific metric.
     * @param metric Metric type.
     * @param context Athlete context (FTP, HR max, etc.).
     * @return Zone definitions for the metric.
     */
    static std::vector<Zone> zonesForMetric(ColorMetric metric,
                                            const ColorContext& context);

    /**
     * @brief Computes time/distance distribution across zones.
     * @param rideData Activity data.
     * @param metric Metric type.
     * @param context Athlete context.
     * @return Distribution of time spent in each zone.
     */
    static std::vector<ZoneDistribution> computeDistribution(
        const RideData& rideData,
        ColorMetric metric,
        const ColorContext& context);

    /**
     * @brief Checks if activity has samples for a metric.
     * @param rideData Activity data.
     * @param metric Metric type.
     * @return True if samples exist.
     */
    static bool hasMetricSamples(const RideData& rideData, ColorMetric metric);
};