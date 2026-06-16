// SPDX-License-Identifier: GPL-3


#pragma once

/**
 * @brief Bitmask flags for analysis completion tracking.
 *
 * Flags are stored in activities.analysis_flags to track which analysis
 * products have been computed, allowing the UI to warn on outdated results.
 */
namespace ActivityAnalysisFlags
{
    /**
     * @brief No analysis flags set.
     */
    constexpr int None          = 0;

    /**
     * @brief Climb detection has been computed.
     */
    constexpr int HasClimbs     = 1 << 0;

    /**
     * @brief Interval detection has been computed.
     */
    constexpr int HasIntervals  = 1 << 1;

    /**
     * @brief Power curve has been computed.
     */
    constexpr int HasPowerCurve = 1 << 2;

    /**
     * @brief Power histogram has been computed.
     */
    constexpr int HasHistogram  = 1 << 3;

    /**
     * @brief Video export has been created.
     */
    constexpr int HasVideo      = 1 << 4;
}