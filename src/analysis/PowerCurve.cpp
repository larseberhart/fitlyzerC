// SPDX-License-Identifier: GPL-3


#include "PowerCurve.h"

#include <cmath>
#include <vector>

std::vector<double> PowerCurve::standardDurations()
{
    return {
        5, 10, 15, 30, 60,
        120, 180, 300, 600,
        1200, 1800, 3600, 5400, 7200
    };
}

double PowerCurve::bestMeanPower(
    const RideData& rideData, double durationSeconds)
{
    const auto& records = rideData.records;
    if (records.empty()) return 0.0;

    std::vector<double> power;
    power.reserve(records.size());
    for (const auto& r : records)
        power.push_back(r.hasPower ? r.power : 0.0);

    const int n      = static_cast<int>(power.size());
    const int window = static_cast<int>(std::round(durationSeconds));
    if (window <= 0 || window > n) return 0.0;

    double sum = 0.0;
    for (int i = 0; i < window; ++i)
        sum += power[i];

    double best = sum;
    for (int i = window; i < n; ++i)
    {
        sum += power[i] - power[i - window];
        if (sum > best) best = sum;
    }

    return best / window;
}

std::vector<PowerCurvePoint> PowerCurve::computeStandard(
    const RideData& rideData)
{
    const auto durations = standardDurations();
    std::vector<PowerCurvePoint> result;
    result.reserve(durations.size());

    for (double d : durations)
    {
        const double p = bestMeanPower(rideData, d);
        if (p > 0.0)
            result.push_back({ d, p });
    }

    return result;
}