#include "SmoothingEngine.h"

#include <cmath>
#include <limits>

std::vector<double> SmoothingEngine::smoothPower(const RideData& ride, int seconds)
{
    const int n = static_cast<int>(ride.records.size());
    std::vector<double> out(static_cast<size_t>(n), std::numeric_limits<double>::quiet_NaN());
    if (n == 0)
        return out;

    if (seconds <= 0)
    {
        for (int i = 0; i < n; ++i)
        {
            const RideRecord& r = ride.records[static_cast<size_t>(i)];
            if (r.hasPower && r.power > 0.0)
                out[static_cast<size_t>(i)] = r.power;
        }
        return out;
    }

    const double halfWindow = seconds / 2.0;
    std::vector<double> prefixSum(static_cast<size_t>(n) + 1, 0.0);
    std::vector<int> prefixCount(static_cast<size_t>(n) + 1, 0);

    for (int i = 0; i < n; ++i)
    {
        const RideRecord& r = ride.records[static_cast<size_t>(i)];
        const bool valid = r.hasPower && r.power > 0.0;
        prefixSum[static_cast<size_t>(i) + 1] = prefixSum[static_cast<size_t>(i)] + (valid ? r.power : 0.0);
        prefixCount[static_cast<size_t>(i) + 1] = prefixCount[static_cast<size_t>(i)] + (valid ? 1 : 0);
    }

    int left = 0;
    int right = 0;
    for (int i = 0; i < n; ++i)
    {
        const RideRecord& r = ride.records[static_cast<size_t>(i)];
        const double t = r.elapsedSeconds;

        while (left < n && ride.records[static_cast<size_t>(left)].elapsedSeconds < t - halfWindow)
            ++left;
        while (right < n && ride.records[static_cast<size_t>(right)].elapsedSeconds <= t + halfWindow)
            ++right;

        if (!(r.hasPower && r.power > 0.0))
            continue;

        const double sum = prefixSum[static_cast<size_t>(right)] - prefixSum[static_cast<size_t>(left)];
        const int count = prefixCount[static_cast<size_t>(right)] - prefixCount[static_cast<size_t>(left)];
        if (count > 0)
            out[static_cast<size_t>(i)] = sum / static_cast<double>(count);
    }

    return out;
}
