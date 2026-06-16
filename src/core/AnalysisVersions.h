// SPDX-License-Identifier: GPL-3


#pragma once

/**
 * @brief Algorithm version tracking for analysis results.
 *
 * Version constants are incremented when algorithms change
 * to track data compatibility and warn on outdated results.
 */
namespace AnalysisVersions
{
    /**
     * @brief Climb detection algorithm version.
     */
    constexpr int ClimbDetection    = 1;

    /**
     * @brief Interval detection algorithm version.
     */
    constexpr int IntervalDetection = 1;
}