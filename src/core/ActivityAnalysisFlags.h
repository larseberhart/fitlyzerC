#pragma once

/// Bitmask flags stored in activities.analysis_flags to track which analysis
/// products have ever been computed for an activity.
namespace ActivityAnalysisFlags
{
    constexpr int None          = 0;
    constexpr int HasClimbs     = 1 << 0;
    constexpr int HasIntervals  = 1 << 1;
    constexpr int HasPowerCurve = 1 << 2;
    constexpr int HasHistogram  = 1 << 3;
    constexpr int HasVideo      = 1 << 4;
}
