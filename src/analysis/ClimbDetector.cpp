#include "ClimbDetector.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace
{
bool isFinite(double value)
{
    return std::isfinite(value);
}

double segmentMeters(double lat1, double lon1, double lat2, double lon2)
{
    const double dLat = (lat2 - lat1) * std::numbers::pi_v<double> / 180.0;
    const double dLon = (lon2 - lon1) * std::numbers::pi_v<double> / 180.0;
    const double mLat = (lat1 + lat2) / 2.0 * std::numbers::pi_v<double> / 180.0;
    return std::hypot(dLat, dLon * std::cos(mLat)) * 6371000.0;
}
} // namespace

std::vector<double> ClimbDetector::buildCumulativeDistanceMeters(const RideData& rideData)
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

std::vector<double> ClimbDetector::smoothAltitudeByDistance(
    const RideData& rideData,
    const std::vector<double>& cumulativeDistanceMeters,
    double windowMeters)
{
    const size_t n = rideData.records.size();
    std::vector<double> smoothed(n, std::numeric_limits<double>::quiet_NaN());
    if (n == 0)
        return smoothed;

    if (windowMeters <= 0.0)
    {
        for (size_t i = 0; i < n; ++i)
        {
            const RideRecord& r = rideData.records[i];
            if (r.hasAltitude)
                smoothed[i] = r.altitude;
        }
        return smoothed;
    }

    const double halfWindow = windowMeters / 2.0;
    size_t left = 0;
    size_t right = 0;

    for (size_t i = 0; i < n; ++i)
    {
        const double center = cumulativeDistanceMeters[i];
        while (left < n && cumulativeDistanceMeters[left] < center - halfWindow)
            ++left;
        while (right < n && cumulativeDistanceMeters[right] <= center + halfWindow)
            ++right;

        double sumAlt = 0.0;
        int countAlt = 0;
        for (size_t j = left; j < right; ++j)
        {
            const RideRecord& r = rideData.records[j];
            if (!r.hasAltitude)
                continue;
            sumAlt += r.altitude;
            ++countAlt;
        }

        if (countAlt > 0)
            smoothed[i] = sumAlt / static_cast<double>(countAlt);
    }

    return smoothed;
}

std::vector<double> ClimbDetector::computeLocalGradientPct(
    const std::vector<double>& smoothedAltitudeMeters,
    const std::vector<double>& cumulativeDistanceMeters,
    double halfWindowMeters)
{
    const size_t n = smoothedAltitudeMeters.size();
    std::vector<double> gradient(n, std::numeric_limits<double>::quiet_NaN());
    if (n == 0)
        return gradient;

    if (halfWindowMeters <= 0.0)
        halfWindowMeters = 1.0;

    size_t left = 0;
    size_t right = 0;

    for (size_t i = 0; i < n; ++i)
    {
        const double center = cumulativeDistanceMeters[i];
        while (left + 1 < n && cumulativeDistanceMeters[left + 1] <= center - halfWindowMeters)
            ++left;
        while (right < n && cumulativeDistanceMeters[right] < center + halfWindowMeters)
            ++right;
        if (right >= n)
            right = n - 1;

        if (!isFinite(smoothedAltitudeMeters[left]) || !isFinite(smoothedAltitudeMeters[right]))
            continue;

        const double distDiff = cumulativeDistanceMeters[right] - cumulativeDistanceMeters[left];
        if (distDiff <= 0.0)
            continue;

        const double altDiff = smoothedAltitudeMeters[right] - smoothedAltitudeMeters[left];
        gradient[i] = (altDiff / distDiff) * 100.0;
    }

    return gradient;
}

std::vector<Climb> ClimbDetector::detect(const RideData& rideData, const Config& config)
{
    const int n = static_cast<int>(rideData.records.size());
    if (n < 3)
        return {};

    bool hasAltitude = false;
    for (const RideRecord& r : rideData.records)
    {
        if (r.hasAltitude)
        {
            hasAltitude = true;
            break;
        }
    }
    if (!hasAltitude)
        return {};

    const std::vector<double> cumulativeDistanceMeters = buildCumulativeDistanceMeters(rideData);
    const std::vector<double> smoothedAltitudeMeters = smoothAltitudeByDistance(
        rideData,
        cumulativeDistanceMeters,
        config.elevationSmoothingMeters);
    const std::vector<double> localGradientPct = computeLocalGradientPct(
        smoothedAltitudeMeters,
        cumulativeDistanceMeters,
        config.gradientWindowHalfMeters);

    std::vector<Climb> climbs;
    int i = 0;

    while (i < n)
    {
        int localMinIdx = -1;
        int climbStart = -1;
        double localMinAlt = std::numeric_limits<double>::quiet_NaN();

        const double startGainRequiredMeters =
            (config.startGradient / 100.0) * config.minStartContinuousDistanceMeters;

        for (; i < n; ++i)
        {
            const double alt = smoothedAltitudeMeters[static_cast<size_t>(i)];
            if (!isFinite(alt))
                continue;

            if (localMinIdx < 0)
            {
                localMinIdx = i;
                localMinAlt = alt;
                continue;
            }

            if (alt <= localMinAlt)
            {
                localMinIdx = i;
                localMinAlt = alt;
                continue;
            }

            const double gainFromMin = alt - localMinAlt;
            const double distFromMin =
                cumulativeDistanceMeters[static_cast<size_t>(i)] - cumulativeDistanceMeters[static_cast<size_t>(localMinIdx)];

            if (distFromMin >= config.minStartContinuousDistanceMeters &&
                gainFromMin >= startGainRequiredMeters)
            {
                climbStart = localMinIdx;
                break;
            }
        }

        if (climbStart < 0)
            break;

        int peakIdx = climbStart;
        double peakAlt = isFinite(smoothedAltitudeMeters[static_cast<size_t>(climbStart)])
            ? smoothedAltitudeMeters[static_cast<size_t>(climbStart)]
            : rideData.records[static_cast<size_t>(climbStart)].altitude;

        int k = climbStart + 1;
        for (; k < n; ++k)
        {
            const double alt = smoothedAltitudeMeters[static_cast<size_t>(k)];
            if (!isFinite(alt))
                continue;

            if (alt >= peakAlt)
            {
                peakAlt = alt;
                peakIdx = k;
                continue;
            }

            const double dipMeters = peakAlt - alt;
            const double dipDistance =
                cumulativeDistanceMeters[static_cast<size_t>(k)] - cumulativeDistanceMeters[static_cast<size_t>(peakIdx)];

            if (dipMeters > config.maxDipMeters || dipDistance > config.maxDipDistanceMeters)
                break;
        }

        const int climbEnd = peakIdx;
        i = std::max(k, climbEnd + 1);

        if (climbEnd <= climbStart)
            continue;

        Climb climb = ClimbMetrics::build(
            rideData,
            cumulativeDistanceMeters,
            smoothedAltitudeMeters,
            localGradientPct,
            climbStart,
            climbEnd,
            static_cast<int>(climbs.size()) + 1);

        if (climb.lengthKm < config.minLengthKm)
            continue;
        if (climb.elevationGainM < config.minElevationGainM)
            continue;
        if (climb.averageGradient < config.minAverageGradient)
            continue;

        climbs.push_back(climb);
    }

    return climbs;
}
