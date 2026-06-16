// SPDX-License-Identifier: GPL-3


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