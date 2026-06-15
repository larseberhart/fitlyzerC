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

double clampValue(double value, double lo, double hi)
{
    return std::max(lo, std::min(hi, value));
}

double segmentMeters(double lat1, double lon1, double lat2, double lon2)
{
    const double dLat = (lat2 - lat1) * std::numbers::pi_v<double> / 180.0;
    const double dLon = (lon2 - lon1) * std::numbers::pi_v<double> / 180.0;
    const double mLat = (lat1 + lat2) / 2.0 * std::numbers::pi_v<double> / 180.0;
    return std::hypot(dLat, dLon * std::cos(mLat)) * 6371000.0;
}

std::vector<double> buildFilteredAltitudeMeters(
    const RideData& rideData,
    const std::vector<double>& cumulativeDistanceMeters,
    double maxVerticalSpeedMps)
{
    const size_t n = rideData.records.size();
    std::vector<double> filtered(n, std::numeric_limits<double>::quiet_NaN());
    for (size_t i = 0; i < n; ++i)
    {
        if (rideData.records[i].hasAltitude)
            filtered[i] = rideData.records[i].altitude;
    }

    if (n < 3 || maxVerticalSpeedMps <= 0.0)
        return filtered;

    for (size_t i = 1; i + 1 < n; ++i)
    {
        if (!isFinite(filtered[i - 1]) || !isFinite(filtered[i]) || !isFinite(filtered[i + 1]))
            continue;

        const RideRecord& prev = rideData.records[i - 1];
        const RideRecord& now = rideData.records[i];
        const RideRecord& next = rideData.records[i + 1];

        const double dtPrev = now.elapsedSeconds - prev.elapsedSeconds;
        const double dtNext = next.elapsedSeconds - now.elapsedSeconds;
        if (dtPrev <= 0.0 || dtNext <= 0.0)
            continue;

        const double vsPrev = (filtered[i] - filtered[i - 1]) / dtPrev;
        const double vsNext = (filtered[i + 1] - filtered[i]) / dtNext;
        const bool oppositeDirections = (vsPrev > 0.0 && vsNext < 0.0) || (vsPrev < 0.0 && vsNext > 0.0);
        if (!oppositeDirections)
            continue;

        if (std::abs(vsPrev) <= maxVerticalSpeedMps || std::abs(vsNext) <= maxVerticalSpeedMps)
            continue;

        const double spanDist = cumulativeDistanceMeters[i + 1] - cumulativeDistanceMeters[i - 1];
        const double localDist = cumulativeDistanceMeters[i] - cumulativeDistanceMeters[i - 1];
        const double ratio = spanDist > 0.0 ? clampValue(localDist / spanDist, 0.0, 1.0) : 0.5;
        filtered[i] = filtered[i - 1] + (filtered[i + 1] - filtered[i - 1]) * ratio;
    }

    return filtered;
}

double estimateSpeedKmhAt(
    const RideData& rideData,
    const std::vector<double>& cumulativeDistanceMeters,
    int index)
{
    const int n = static_cast<int>(rideData.records.size());
    if (index < 0 || index >= n)
        return std::numeric_limits<double>::quiet_NaN();

    const RideRecord& r = rideData.records[static_cast<size_t>(index)];
    if (r.hasSpeed && isFinite(r.speed) && r.speed >= 0.0)
        return r.speed;

    const int prev = std::max(0, index - 1);
    const int next = std::min(n - 1, index + 1);
    if (next == prev)
        return std::numeric_limits<double>::quiet_NaN();

    const double dt = rideData.records[static_cast<size_t>(next)].elapsedSeconds -
        rideData.records[static_cast<size_t>(prev)].elapsedSeconds;
    const double dd = cumulativeDistanceMeters[static_cast<size_t>(next)] -
        cumulativeDistanceMeters[static_cast<size_t>(prev)];
    if (dt <= 0.0 || dd < 0.0)
        return std::numeric_limits<double>::quiet_NaN();

    return (dd / dt) * 3.6;
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
    double windowMeters,
    const std::vector<double>& adaptiveWindowMeters,
    double altitudeSpikeVerticalSpeedMps)
{
    const size_t n = rideData.records.size();
    std::vector<double> smoothed(n, std::numeric_limits<double>::quiet_NaN());
    if (n == 0)
        return smoothed;

    const std::vector<double> filteredAltitudeMeters = buildFilteredAltitudeMeters(
        rideData,
        cumulativeDistanceMeters,
        altitudeSpikeVerticalSpeedMps);

    if (windowMeters <= 0.0)
    {
        for (size_t i = 0; i < n; ++i)
        {
            if (isFinite(filteredAltitudeMeters[i]))
                smoothed[i] = filteredAltitudeMeters[i];
        }
        return smoothed;
    }

    size_t left = 0;
    size_t right = 0;

    for (size_t i = 0; i < n; ++i)
    {
        const double sampleWindow =
            (adaptiveWindowMeters.size() == n && isFinite(adaptiveWindowMeters[i]) && adaptiveWindowMeters[i] > 0.0)
            ? adaptiveWindowMeters[i]
            : windowMeters;
        const double halfWindow = std::max(1.0, sampleWindow / 2.0);
        const double center = cumulativeDistanceMeters[i];
        while (left < n && cumulativeDistanceMeters[left] < center - halfWindow)
            ++left;
        while (right < n && cumulativeDistanceMeters[right] <= center + halfWindow)
            ++right;

        double sumAlt = 0.0;
        int countAlt = 0;
        for (size_t j = left; j < right; ++j)
        {
            if (!isFinite(filteredAltitudeMeters[j]))
                continue;
            sumAlt += filteredAltitudeMeters[j];
            ++countAlt;
        }

        if (countAlt > 0)
            smoothed[i] = sumAlt / static_cast<double>(countAlt);
    }

    return smoothed;
}

std::vector<double> ClimbDetector::computeAdaptiveSmoothingWindowsMeters(
    const RideData& rideData,
    const std::vector<double>& cumulativeDistanceMeters,
    const Config& config)
{
    const size_t n = rideData.records.size();
    std::vector<double> windows(n, std::max(1.0, config.elevationSmoothingMeters));
    if (n == 0)
        return windows;

    if (!config.useAdaptiveSmoothing)
        return windows;

    const double minWin = std::max(1.0, std::min(config.adaptiveSmoothingMinMeters, config.adaptiveSmoothingMaxMeters));
    const double maxWin = std::max(minWin, std::max(config.adaptiveSmoothingMinMeters, config.adaptiveSmoothingMaxMeters));

    for (int i = 0; i < static_cast<int>(n); ++i)
    {
        const double speedKmh = estimateSpeedKmhAt(rideData, cumulativeDistanceMeters, i);
        if (!isFinite(speedKmh))
        {
            windows[static_cast<size_t>(i)] = clampValue(config.elevationSmoothingMeters, minWin, maxWin);
            continue;
        }

        const double targetWindow = 20.0 + speedKmh * 1.8;
        windows[static_cast<size_t>(i)] = clampValue(targetWindow, minWin, maxWin);
    }

    return windows;
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
    const std::vector<double> adaptiveSmoothingWindows = computeAdaptiveSmoothingWindowsMeters(
        rideData,
        cumulativeDistanceMeters,
        config);
    const std::vector<double> smoothedAltitudeMeters = smoothAltitudeByDistance(
        rideData,
        cumulativeDistanceMeters,
        config.elevationSmoothingMeters,
        adaptiveSmoothingWindows,
        config.altitudeSpikeVerticalSpeedMps);
    const std::vector<double> localGradientPct = computeLocalGradientPct(
        smoothedAltitudeMeters,
        cumulativeDistanceMeters,
        config.gradientWindowHalfMeters);

    std::vector<Climb> climbs;
    int i = 0;

    while (i < n)
    {
        int triggerIdx = -1;
        for (; i < n; ++i)
        {
            const double gradient = localGradientPct[static_cast<size_t>(i)];
            if (isFinite(gradient) && gradient >= config.startGradient)
            {
                triggerIdx = i;
                break;
            }
        }

        if (triggerIdx < 0)
            break;

        int localMinIdx = triggerIdx;
        double localMinAlt = smoothedAltitudeMeters[static_cast<size_t>(triggerIdx)];
        if (!isFinite(localMinAlt))
        {
            i = triggerIdx + 1;
            continue;
        }

        // Backtrack up to 1 km to anchor the climb start at the local minimum.
        for (int b = triggerIdx - 1; b >= 0; --b)
        {
            const double backtrackDist =
                cumulativeDistanceMeters[static_cast<size_t>(triggerIdx)] - cumulativeDistanceMeters[static_cast<size_t>(b)];
            if (backtrackDist > 1000.0)
                break;

            const double alt = smoothedAltitudeMeters[static_cast<size_t>(b)];
            if (isFinite(alt) && alt <= localMinAlt)
            {
                localMinAlt = alt;
                localMinIdx = b;
            }
        }

        const int climbStart = localMinIdx;

        double sustainedDistance = 0.0;
        double interruptionDistance = 0.0;
        int confirmedIdx = -1;
        for (int j = std::max(triggerIdx, climbStart + 1); j < n; ++j)
        {
            const double segmentDist =
                cumulativeDistanceMeters[static_cast<size_t>(j)] - cumulativeDistanceMeters[static_cast<size_t>(j - 1)];
            const double gradient = localGradientPct[static_cast<size_t>(j)];

            if (isFinite(gradient) && gradient >= config.continueGradient)
            {
                sustainedDistance += std::max(0.0, segmentDist);
                interruptionDistance = 0.0;
            }
            else
            {
                interruptionDistance += std::max(0.0, segmentDist);
            }

            if (interruptionDistance > config.maxStartInterruptionMeters)
                break;

            if (sustainedDistance >= config.minStartContinuousDistanceMeters)
            {
                confirmedIdx = j;
                break;
            }
        }

        if (confirmedIdx < 0)
        {
            i = triggerIdx + 1;
            continue;
        }

        int peakIdx = climbStart;
        double peakAlt = localMinAlt;
        double plateauDistance = 0.0;

        int k = confirmedIdx;
        for (; k < n; ++k)
        {
            const double alt = smoothedAltitudeMeters[static_cast<size_t>(k)];
            if (!isFinite(alt))
                continue;

            const double gradient = localGradientPct[static_cast<size_t>(k)];
            const double segmentDist = (k > 0)
                ? std::max(0.0, cumulativeDistanceMeters[static_cast<size_t>(k)] - cumulativeDistanceMeters[static_cast<size_t>(k - 1)])
                : 0.0;

            if (alt >= peakAlt)
            {
                peakAlt = alt;
                peakIdx = k;
                plateauDistance = 0.0;
                continue;
            }

            const double dipMeters = peakAlt - alt;
            const double dipDistance =
                cumulativeDistanceMeters[static_cast<size_t>(k)] - cumulativeDistanceMeters[static_cast<size_t>(peakIdx)];

            const double gainSoFar = std::max(0.0, peakAlt - localMinAlt);
            const double allowedDipMeters = std::max(
                config.maxDipMeters,
                gainSoFar * (config.maxDipPctOfGain / 100.0));

            if (isFinite(gradient) && gradient >= config.continueGradient)
            {
                plateauDistance = 0.0;
                continue;
            }

            if (isFinite(gradient) && gradient >= config.plateauMinGradient)
                plateauDistance += segmentDist;
            else
                plateauDistance = config.maxPlateauDistanceMeters + 1.0;

            const bool exceededDip =
                dipMeters > allowedDipMeters && dipDistance > config.maxDipDistanceMeters;
            const bool plateauExpired =
                plateauDistance > config.maxPlateauDistanceMeters &&
                (!isFinite(gradient) || gradient < config.endGradient);

            if (exceededDip || plateauExpired)
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
