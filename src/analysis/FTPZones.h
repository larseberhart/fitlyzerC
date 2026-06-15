// SPDX-License-Identifier: GPL-3

/**
 * @file FTPZones.h
 * @brief Analysis component for FTPZones.
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
#include <string>

#include "fit/RideData.h"

/**
 * @brief Represents a single power zone with time spent.
 */
struct PowerZone
{
    /// @brief Zone number (1-7).
    int         number;

    /// @brief Zone name (Z1, Z2, etc.).
    std::string name;

    /// @brief Lower bound as fraction of FTP (inclusive).
    double      lowerPct;

    /// @brief Upper bound as fraction of FTP (exclusive), -1.0 for unbounded.
    double      upperPct;

    /// @brief Time spent in zone in seconds.
    double      seconds = 0.0;
};

/**
 * @brief Analyzes time spent in FTP-based power zones.
 */
class FTPZones
{
public:
    /// @brief Number of FTP power zones (always 7 for standard zones).
    static constexpr int kZoneCount = 7;

    /// @brief Array container type for zone definitions.
    using ZoneArray = std::array<PowerZone, kZoneCount>;

    /**
     * @brief Returns the default power zone definitions.
     * @return Array of standard FTP-based zones.
     */
    static ZoneArray defaultZones();

    /**
     * @brief Computes power zones for an activity.
     * @param rideData Activity data.
     * @param ftp Athlete functional threshold power in watts.
     * @return Array of zones with time spent in each.
     */
    static ZoneArray compute(const RideData& rideData, double ftp);
};