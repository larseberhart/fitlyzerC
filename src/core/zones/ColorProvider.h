// SPDX-License-Identifier: GPL-3

/**
 * @file ColorProvider.h
 * @brief Training zone support for ColorProvider.
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

#include <vector>

#include "ZoneDefinition.h"

/**
 * @brief Provides color mapping for athletic metrics.
 *
 * Generates colors for power, heart rate, cadence, speed, and altitude based on zones.
 */
class ColorProvider
{
public:
    /**
     * @brief Gets color for a metric value.
     * @param metric Metric type.
     * @param value Value to color.
     * @param context Athlete context (FTP, HR max, etc.).
     * @return Color for the value.
     */
    static QColor colorForMetric(ColorMetric metric,
                                 double value,
                                 const ColorContext& context);

    /**
     * @brief Gets color for a power value.
     * @param watts Power in watts.
     * @param ftp Functional threshold power.
     * @return Color for power.
     */
    static QColor colorForPower(double watts, int ftp);

    /**
     * @brief Gets color for a heart rate value.
     * @param bpm Heart rate in beats per minute.
     * @param heartRateMax Maximum heart rate.
     * @param heartRateRest Resting heart rate (default 60).
     * @param thresholdHeartRate Threshold HR (default 0).
     * @return Color for heart rate.
     */
    static QColor colorForHeartRate(double bpm,
                                    int heartRateMax,
                                    int heartRateRest = 60,
                                    int thresholdHeartRate = 0);

    /**
     * @brief Gets color for a cadence value.
     * @param cadence Cadence in RPM.
     * @return Color for cadence.
     */
    static QColor colorForCadence(double cadence);

    /**
     * @brief Gets color for a speed value.
     * @param speedKmh Speed in km/h.
     * @return Color for speed.
     */
    static QColor colorForSpeed(double speedKmh);

    /**
     * @brief Gets color for an altitude value.
     * @param altitude Altitude in meters.
     * @param minAltitude Minimum altitude in range.
     * @param maxAltitude Maximum altitude in range.
     * @return Color for altitude.
     */
    static QColor colorForAltitude(double altitude,
                                   double minAltitude,
                                   double maxAltitude);

    /**
     * @brief Gets color for a value based on zone definitions.
     * @param value Value to color.
     * @param zones Zone definitions.
     * @param fallback Color to use if value is outside all zones.
     * @return Color for the value.
     */
    static QColor colorForValue(double value,
                                const std::vector<Zone>& zones,
                                const QColor& fallback = QColor("#94a3b8"));
};