// SPDX-License-Identifier: GPL-3

/**
 * @file SmoothingEngine.h
 * @brief Analysis component for SmoothingEngine.
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
 * @brief Provides data smoothing algorithms for activity metrics.
 */
class SmoothingEngine
{
public:
    /**
     * @brief Computes smoothed power series using rolling average.
     * @param ride Activity data.
     * @param seconds Window width in seconds for rolling average.
     * @return Smoothed power series (watts); NaN for invalid samples.
     */
    static std::vector<double> smoothPower(const RideData& ride, int seconds);
};