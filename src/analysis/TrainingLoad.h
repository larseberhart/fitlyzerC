// SPDX-License-Identifier: GPL-3

/**
 * @file TrainingLoad.h
 * @brief Analysis component for TrainingLoad.
 *
 * Implements analysis logic used to compute cycling metrics, detect patterns, and derive activity insights.
 *
 * Responsibilities:
 * - Provide analysis-specific functionality for activity processing
 *
 * @author Lars EBERHART
 */

#pragma once

#include <vector>

#include "fit/RideData.h"

/**
 * @brief Daily training stress score value.
 */
struct DailyLoadPoint
{
    /// @brief Training stress score for day.
    double tss = 0.0;
};

/**
 * @brief Fitness metrics for a single day.
 *
 * Represents Acute Training Load (ATL), Chronic Training Load (CTL),
 * and Training Stress Balance (TSB) calculated from Training Stress Score.
 */
struct FitnessMetricsPoint
{
    /// @brief Acute Training Load (7-day EWMA).
    double atl = 0.0;

    /// @brief Chronic Training Load (42-day EWMA).
    double ctl = 0.0;

    /// @brief Training Stress Balance (ctl - atl).
    double tsb = 0.0;
};

/**
 * @brief Computes training load metrics from activity power data.
 *
 * Provides static methods to calculate normalized power, intensity factor,
 * training stress score, and fitness timeline metrics (ATL, CTL, TSB).
 */
class TrainingLoad
{
public:
    /**
     * @brief Calculates normalized power for an activity.
     * @param rideData Activity data to analyze.
     * @return Normalized power in watts.
     */
    static double normalizedPower(const RideData& rideData);

    /**
     * @brief Calculates intensity factor.
     * @param np Normalized power in watts.
     * @param ftp Functional threshold power in watts.
     * @return Intensity factor (NP/FTP ratio).
     */
    static double intensityFactor(double np, double ftp);

    /**
     * @brief Calculates training stress score for an activity.
     * @param durationSeconds Activity duration in seconds.
     * @param np Normalized power in watts.
     * @param ftp Functional threshold power in watts.
     * @return Training stress score.
     */
    static double trainingStressScore(
        double durationSeconds, double np, double ftp);

    /**
     * @brief Computes ATL/CTL/TSB sequence from daily training stress scores.
     * @param dailyLoads Daily TSS values.
     * @param atlDays Acute training load window in days (default 7).
     * @param ctlDays Chronic training load window in days (default 42).
     * @return Timeline of fitness metrics.
     */
    static std::vector<FitnessMetricsPoint> fitnessTimeline(
        const std::vector<DailyLoadPoint>& dailyLoads,
        int atlDays = 7,
        int ctlDays = 42);
};