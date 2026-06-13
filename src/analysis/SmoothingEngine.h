#pragma once

#include <vector>

#include "fit/RideData.h"

class SmoothingEngine
{
public:
    // Returns a per-sample smoothed power series.
    // Invalid/absent power samples are returned as NaN.
    static std::vector<double> smoothPower(const RideData& ride, int seconds);
};
