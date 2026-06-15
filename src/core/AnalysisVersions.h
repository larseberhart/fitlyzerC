#pragma once

/// Algorithm version constants.  Increment each counter whenever the
/// corresponding detection algorithm changes in a way that may produce
/// different results from the same input data.  Stored in every climbs /
/// intervals row so the UI can warn when stored data was produced by an
/// outdated algorithm.
namespace AnalysisVersions
{
    constexpr int ClimbDetection    = 1;
    constexpr int IntervalDetection = 1;
}
