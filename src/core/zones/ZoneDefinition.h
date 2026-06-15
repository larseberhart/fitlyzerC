// SPDX-License-Identifier: GPL-3

/**
 * @file ZoneDefinition.h
 * @brief Training zone support for ZoneDefinition.
 *
 * Implements training-zone definitions, calculations, and related visual behavior for zone-based analysis.
 *
 * Responsibilities:
 * - Provide zone definitions, calculations, or zone-related utilities
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QColor>
#include <QString>

/**
 * @brief Metric types used for zone coloring in charts and displays.
 */
enum class ColorMetric
{
    None,
    Power,
    HeartRate,
    Cadence,
    Speed,
    Altitude,
    Gradient
};

/**
 * @brief Represents a single training or power zone with bounds and color.
 */
struct Zone
{
    /// @brief Zone name.
    QString name;

    /// @brief Minimum zone value (inclusive).
    double minValue = 0.0;

    /// @brief Maximum zone value (exclusive).
    double maxValue = 0.0;

    /// @brief Zone color for display.
    QColor color;
};

/**
 * @brief Distribution of time or distance spent in a specific zone.
 */
struct ZoneDistribution
{
    /// @brief Zone definition.
    Zone zone;

    /// @brief Time spent in zone (seconds).
    double seconds = 0.0;

    /// @brief Percentage of total time in zone.
    double percent = 0.0;
};

/**
 * @brief Context parameters for determining zone colors based on athlete metrics.
 */
struct ColorContext
{
    /// @brief Functional threshold power in watts.
    int ftp = 250;

    /// @brief Maximum heart rate in bpm.
    int heartRateMax = 190;

    /// @brief Resting heart rate in bpm.
    int heartRateRest = 60;

    /// @brief Heart rate threshold in bpm (for HR zones).
    int thresholdHeartRate = 0;

    /// @brief Minimum altitude in meters (for altitude coloring).
    double altitudeMin = 0.0;

    /// @brief Maximum altitude in meters (for altitude coloring).
    double altitudeMax = 0.0;

    /// @brief Whether altitude range is valid for coloring.
    bool hasAltitudeRange = false;
};

/**
 * @brief Returns human-readable display name for a color metric.
 * @param metric The metric to get the display name for.
 * @return Display name string.
 */
QString colorMetricDisplayName(ColorMetric metric);

/**
 * @brief Returns unit string for a color metric.
 * @param metric The metric to get the unit for.
 * @return Unit string (e.g., "W", "bpm", "rpm").
 */
QString colorMetricUnit(ColorMetric metric);