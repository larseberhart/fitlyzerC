// SPDX-License-Identifier: GPL-3

#include "ClimbCandidateFinder.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
double segmentMeters(double lat1, double lon1, double lat2, double lon2)
{
    const double dLat = (lat2 - lat1) * std::numbers::pi_v<double> / 180.0;
    const double dLon = (lon2 - lon1) * std::numbers::pi_v<double> / 180.0;
    const double mLat = (lat1 + lat2) / 2.0 * std::numbers::pi_v<double> / 180.0;
    return std::hypot(dLat, dLon * std::cos(mLat)) * 6371000.0;
}
}

namespace ClimbCandidateFinder
{
std::vector<double> buildCumulativeDistanceMeters(const RideData& rideData)
{
    const size_t n = rideData.records.size();
    std::vector<double> cumulative(n, 0.0);
    if (n == 0)
        return cumulative;

    bool prevHasGps = false;
    double prevLat = 0.0;
    double prevLon = 0.0;

    for (size_t i = 1; i < n; ++i)
    {
        const RideRecord& prev = rideData.records[i - 1];
        const RideRecord& now = rideData.records[i];

        double segment = 0.0;
        if (now.hasGps)
        {
            if (prevHasGps)
                segment = segmentMeters(prevLat, prevLon, now.latitude, now.longitude);
            prevHasGps = true;
            prevLat = now.latitude;
            prevLon = now.longitude;
        }
        else if (now.hasSpeed)
        {
            const double dt = std::max(0.0, now.elapsedSeconds - prev.elapsedSeconds);
            const double prevSpeed = prev.hasSpeed ? prev.speed : now.speed;
            const double avgSpeedKmh = (now.speed + prevSpeed) / 2.0;
            segment = avgSpeedKmh * (dt / 3600.0) * 1000.0;
        }

        cumulative[i] = cumulative[i - 1] + std::max(0.0, segment);
    }

    return cumulative;
}
}
