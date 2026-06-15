// SPDX-License-Identifier: GPL-3

/**
 * @file ClimbMetrics.cpp
 * @brief Analysis component for ClimbMetrics.
 *
 * Implements analysis logic used to compute cycling metrics, detect patterns, and derive activity insights.
 *
 * Responsibilities:
 * - Provide analysis-specific functionality for activity processing
 *
 * @author Lars EBERHART
 */

#include "ClimbMetrics.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
struct SliceStats
{
    double avgPower = 0.0;
    double avgHeartRate = 0.0;
    double avgCadence = 0.0;
};

bool isFinite(double value)
{
    return std::isfinite(value);
}

double computeLinearRegressionSlope(
    const std::vector<double>& x,
    const std::vector<double>& y,
    double* interceptOut = nullptr)
{
    if (x.size() != y.size() || x.size() < 2)
    {
        if (interceptOut)
            *interceptOut = 0.0;
        return 0.0;
    }

    const double n = static_cast<double>(x.size());
    double sumX = 0.0;
    double sumY = 0.0;
    double sumXY = 0.0;
    double sumXX = 0.0;

    for (size_t i = 0; i < x.size(); ++i)
    {
        sumX += x[i];
        sumY += y[i];
        sumXY += x[i] * y[i];
        sumXX += x[i] * x[i];
    }

    const double denom = (n * sumXX) - (sumX * sumX);
    if (std::abs(denom) < 1e-9)
    {
        if (interceptOut)
            *interceptOut = sumY / n;
        return 0.0;
    }

    const double slope = ((n * sumXY) - (sumX * sumY)) / denom;
    if (interceptOut)
        *interceptOut = (sumY - slope * sumX) / n;
    return slope;
}

QString classifyClimbCategory(double gainM, double difficultyScore)
{
    if (gainM >= 800.0 || difficultyScore >= 4500.0)
        return QStringLiteral("Mountain Pass");
    if (gainM >= 400.0)
        return QStringLiteral("Major Climb");
    if (gainM >= 150.0)
        return QStringLiteral("Climb");
    if (gainM >= 50.0)
        return QStringLiteral("Hill");
    return QStringLiteral("Bump");
}

QString classifyClimbShape(
    const std::vector<double>& gradients,
    double lengthKm,
    double gainM,
    double avgGradient,
    double maxGradient)
{
    if (gradients.size() < 4)
        return QStringLiteral("Steady");

    double sum = 0.0;
    double sumSq = 0.0;
    double minG = gradients.front();
    double maxG = gradients.front();
    int rollingTransitions = 0;
    int prevBand = 0;

    for (double g : gradients)
    {
        sum += g;
        sumSq += g * g;
        minG = std::min(minG, g);
        maxG = std::max(maxG, g);

        const int band = (g >= 4.0) ? 1 : (g <= 1.0 ? -1 : 0);
        if (prevBand != 0 && band != 0 && band != prevBand)
            ++rollingTransitions;
        if (band != 0)
            prevBand = band;
    }

    const double n = static_cast<double>(gradients.size());
    const double mean = sum / n;
    const double variance = std::max(0.0, (sumSq / n) - (mean * mean));
    const double stdDev = std::sqrt(variance);

    const size_t chunk = std::max<size_t>(1, gradients.size() / 3);
    auto meanChunk = [&gradients](size_t begin, size_t end)
    {
        if (begin >= end || end > gradients.size())
            return 0.0;
        double s = 0.0;
        for (size_t i = begin; i < end; ++i)
            s += gradients[i];
        return s / static_cast<double>(end - begin);
    };

    const double firstThird = meanChunk(0, std::min(gradients.size(), chunk));
    const double midThird = meanChunk(std::min(gradients.size(), chunk), std::min(gradients.size(), 2 * chunk));
    const double lastThird = meanChunk(std::min(gradients.size(), 2 * chunk), gradients.size());

    if (lengthKm >= 8.0 && gainM >= 600.0 && avgGradient >= 5.0 && stdDev < 2.2)
        return QStringLiteral("Alpine");
    if (rollingTransitions >= 3 && stdDev >= 2.5)
        return QStringLiteral("Rolling");
    if (maxGradient >= 10.0 && (lastThird - firstThird >= 3.0 || maxGradient - mean >= 5.0))
        return QStringLiteral("Wall");
    if (stdDev >= 2.0 && (midThird + 1.0 < firstThird || midThird + 1.0 < lastThird) && (maxG - minG) >= 5.0)
        return QStringLiteral("Staircase");
    if (stdDev <= 1.5)
        return QStringLiteral("Steady");
    return QStringLiteral("Mixed");
}

SliceStats computeSliceStats(
    const RideData& rideData,
    int startIdx,
    int endIdx,
    const std::vector<double>& basis,
    double sliceStart,
    double sliceEnd)
{
    SliceStats stats;
    if (startIdx < 0 || endIdx < startIdx || endIdx >= static_cast<int>(rideData.records.size()))
        return stats;

    double sumPower = 0.0;
    int countPower = 0;
    double sumHr = 0.0;
    int countHr = 0;
    double sumCadence = 0.0;
    int countCadence = 0;

    for (int i = startIdx; i <= endIdx; ++i)
    {
        const double v = basis[static_cast<size_t>(i)];
        if (!isFinite(v) || v < sliceStart || v > sliceEnd)
            continue;

        const RideRecord& r = rideData.records[static_cast<size_t>(i)];
        if (r.hasPower)
        {
            sumPower += r.power;
            ++countPower;
        }
        if (r.hasHeartRate)
        {
            sumHr += r.heartRate;
            ++countHr;
        }
        if (r.hasCadence)
        {
            sumCadence += r.cadence;
            ++countCadence;
        }
    }

    if (countPower > 0)
        stats.avgPower = sumPower / static_cast<double>(countPower);
    if (countHr > 0)
        stats.avgHeartRate = sumHr / static_cast<double>(countHr);
    if (countCadence > 0)
        stats.avgCadence = sumCadence / static_cast<double>(countCadence);

    return stats;
}

double computeNormalizedPower(const std::vector<double>& power)
{
    if (power.empty())
        return 0.0;

    const int npWin = std::min(30, static_cast<int>(power.size()));
    if (npWin <= 0)
        return 0.0;

    double rollingSum = 0.0;
    for (int i = 0; i < npWin; ++i)
        rollingSum += power[static_cast<size_t>(i)];

    double sum4 = std::pow(rollingSum / static_cast<double>(npWin), 4.0);
    int count = 1;

    for (int i = npWin; i < static_cast<int>(power.size()); ++i)
    {
        rollingSum += power[static_cast<size_t>(i)] - power[static_cast<size_t>(i - npWin)];
        sum4 += std::pow(rollingSum / static_cast<double>(npWin), 4.0);
        ++count;
    }

    return std::pow(sum4 / static_cast<double>(count), 0.25);
}
} // namespace

Climb ClimbMetrics::build(
    const RideData& rideData,
    const std::vector<double>& cumulativeDistanceMeters,
    const std::vector<double>& smoothedAltitudeMeters,
    const std::vector<double>& localGradientPct,
    int startIdx,
    int endIdx,
    int climbOrdinal)
{
    Climb climb;
    if (startIdx < 0 || endIdx < startIdx || endIdx >= static_cast<int>(rideData.records.size()))
        return climb;

    const RideRecord& first = rideData.records[static_cast<size_t>(startIdx)];
    const RideRecord& last = rideData.records[static_cast<size_t>(endIdx)];

    climb.startSeconds = first.elapsedSeconds;
    climb.endSeconds = last.elapsedSeconds;
    climb.durationSeconds = std::max(0.0, climb.endSeconds - climb.startSeconds);

    climb.startDistanceKm = cumulativeDistanceMeters[static_cast<size_t>(startIdx)] / 1000.0;
    climb.endDistanceKm = cumulativeDistanceMeters[static_cast<size_t>(endIdx)] / 1000.0;
    climb.lengthKm = std::max(0.0, climb.endDistanceKm - climb.startDistanceKm);

    double startAlt = first.hasAltitude ? first.altitude : std::numeric_limits<double>::quiet_NaN();
    double endAlt = last.hasAltitude ? last.altitude : std::numeric_limits<double>::quiet_NaN();
    if (isFinite(smoothedAltitudeMeters[static_cast<size_t>(startIdx)]))
        startAlt = smoothedAltitudeMeters[static_cast<size_t>(startIdx)];
    if (isFinite(smoothedAltitudeMeters[static_cast<size_t>(endIdx)]))
        endAlt = smoothedAltitudeMeters[static_cast<size_t>(endIdx)];

    climb.startElevationM = isFinite(startAlt) ? startAlt : 0.0;
    climb.endElevationM = isFinite(endAlt) ? endAlt : climb.startElevationM;

    double cumulativeGainM = 0.0;
    for (int i = startIdx + 1; i <= endIdx; ++i)
    {
        const double prevAlt = smoothedAltitudeMeters[static_cast<size_t>(i - 1)];
        const double currAlt = smoothedAltitudeMeters[static_cast<size_t>(i)];
        if (!isFinite(prevAlt) || !isFinite(currAlt))
            continue;

        cumulativeGainM += std::max(0.0, currAlt - prevAlt);
    }
    climb.elevationGainM = cumulativeGainM;

    const double lengthMeters = climb.lengthKm * 1000.0;
    if (lengthMeters > 0.0)
        climb.averageGradient = (climb.elevationGainM / lengthMeters) * 100.0;

    double maxGradient = 0.0;
    int validGradientCount = 0;
    int climbingGradientCount = 0;
    for (int i = startIdx; i <= endIdx; ++i)
    {
        const double g = localGradientPct[static_cast<size_t>(i)];
        if (isFinite(g))
        {
            maxGradient = std::max(maxGradient, g);
            ++validGradientCount;
            if (g >= 1.5)
                ++climbingGradientCount;
        }
    }
    climb.maximumGradient = maxGradient;
    if (validGradientCount > 0)
    {
        climb.gradePercentile =
            (static_cast<double>(climbingGradientCount) / static_cast<double>(validGradientCount)) * 100.0;
    }

    std::vector<double> powerSeries;
    powerSeries.reserve(static_cast<size_t>(endIdx - startIdx + 1));

    double powerSum = 0.0;
    int powerCount = 0;
    double hrSum = 0.0;
    int hrCount = 0;
    double cadenceSum = 0.0;
    int cadenceCount = 0;
    double speedSum = 0.0;
    int speedCount = 0;

    for (int i = startIdx; i <= endIdx; ++i)
    {
        const RideRecord& r = rideData.records[static_cast<size_t>(i)];
        powerSeries.push_back(r.hasPower ? r.power : 0.0);

        if (r.hasPower)
        {
            powerSum += r.power;
            ++powerCount;
        }
        if (r.hasHeartRate)
        {
            hrSum += r.heartRate;
            ++hrCount;
        }
        if (r.hasCadence)
        {
            cadenceSum += r.cadence;
            ++cadenceCount;
        }
        if (r.hasSpeed)
        {
            speedSum += r.speed;
            ++speedCount;
        }
    }

    if (powerCount > 0)
        climb.averagePower = powerSum / static_cast<double>(powerCount);
    if (hrCount > 0)
        climb.averageHeartRate = hrSum / static_cast<double>(hrCount);
    if (cadenceCount > 0)
        climb.averageCadence = cadenceSum / static_cast<double>(cadenceCount);
    if (speedCount > 0)
        climb.averageSpeed = speedSum / static_cast<double>(speedCount);

    climb.normalizedPower = computeNormalizedPower(powerSeries);

    const double durationHours = climb.durationSeconds / 3600.0;
    if (durationHours > 0.0)
        climb.vam = climb.elevationGainM / durationHours;

    const bool canUseDistanceSlices =
        cumulativeDistanceMeters[static_cast<size_t>(endIdx)] > cumulativeDistanceMeters[static_cast<size_t>(startIdx)];

    std::vector<double> basis(rideData.records.size(), 0.0);
    if (canUseDistanceSlices)
    {
        basis = cumulativeDistanceMeters;
    }
    else
    {
        for (size_t i = 0; i < rideData.records.size(); ++i)
            basis[i] = rideData.records[i].elapsedSeconds;
    }

    const double spanStart = basis[static_cast<size_t>(startIdx)];
    const double spanEnd = basis[static_cast<size_t>(endIdx)];
    const double span = std::max(0.0, spanEnd - spanStart);

    if (span > 0.0)
    {
        const double q0 = spanStart;
        const double q1 = spanStart + span * 0.25;
        const double q2 = spanStart + span * 0.50;
        const double q3 = spanStart + span * 0.75;
        const double q4 = spanEnd;

        const SliceStats firstHalf = computeSliceStats(rideData, startIdx, endIdx, basis, q0, q2);
        const SliceStats secondHalf = computeSliceStats(rideData, startIdx, endIdx, basis, q2, q4);
        climb.powerFirstHalf = firstHalf.avgPower;
        climb.powerSecondHalf = secondHalf.avgPower;

        const SliceStats firstQuarter = computeSliceStats(rideData, startIdx, endIdx, basis, q0, q1);
        const SliceStats secondQuarter = computeSliceStats(rideData, startIdx, endIdx, basis, q1, q2);
        const SliceStats thirdQuarter = computeSliceStats(rideData, startIdx, endIdx, basis, q2, q3);
        const SliceStats lastQuarter = computeSliceStats(rideData, startIdx, endIdx, basis, q3, q4);

        climb.quarterMetrics[0] = { firstQuarter.avgPower, firstQuarter.avgHeartRate, firstQuarter.avgCadence };
        climb.quarterMetrics[1] = { secondQuarter.avgPower, secondQuarter.avgHeartRate, secondQuarter.avgCadence };
        climb.quarterMetrics[2] = { thirdQuarter.avgPower, thirdQuarter.avgHeartRate, thirdQuarter.avgCadence };
        climb.quarterMetrics[3] = { lastQuarter.avgPower, lastQuarter.avgHeartRate, lastQuarter.avgCadence };

        if (firstQuarter.avgPower > 0.0)
        {
            climb.powerFadePct =
                ((lastQuarter.avgPower - firstQuarter.avgPower) / firstQuarter.avgPower) * 100.0;
        }

        if (firstQuarter.avgPower > 0.0 && lastQuarter.avgPower > 0.0 &&
            firstQuarter.avgHeartRate > 0.0 && lastQuarter.avgHeartRate > 0.0)
        {
            const double startRatio = firstQuarter.avgHeartRate / firstQuarter.avgPower;
            const double endRatio = lastQuarter.avgHeartRate / lastQuarter.avgPower;
            if (startRatio > 0.0)
                climb.hrDriftPct = ((endRatio - startRatio) / startRatio) * 100.0;
        }
    }

    std::vector<double> decoupleX;
    std::vector<double> decoupleY;
    decoupleX.reserve(static_cast<size_t>(endIdx - startIdx + 1));
    decoupleY.reserve(static_cast<size_t>(endIdx - startIdx + 1));

    for (int i = startIdx; i <= endIdx; ++i)
    {
        const RideRecord& r = rideData.records[static_cast<size_t>(i)];
        if (!r.hasHeartRate || !r.hasPower || r.power <= 0.0 || r.heartRate <= 0.0)
            continue;

        const double progress = (climb.durationSeconds > 0.0)
            ? (r.elapsedSeconds - climb.startSeconds) / climb.durationSeconds
            : 0.0;
        decoupleX.push_back(std::clamp(progress, 0.0, 1.0));
        decoupleY.push_back(r.heartRate / r.power);
    }

    if (decoupleX.size() >= 6)
    {
        double intercept = 0.0;
        const double slope = computeLinearRegressionSlope(decoupleX, decoupleY, &intercept);
        const double startRatio = intercept;
        const double endRatio = intercept + slope;
        if (startRatio > 0.0)
            climb.aerobicDecouplingPct = ((endRatio - startRatio) / startRatio) * 100.0;
    }
    else
    {
        climb.aerobicDecouplingPct = climb.hrDriftPct;
    }

    climb.hrDriftPct = climb.aerobicDecouplingPct;

    climb.difficultyScore = climb.elevationGainM * climb.averageGradient;

    std::vector<double> climbGradients;
    climbGradients.reserve(static_cast<size_t>(endIdx - startIdx + 1));
    for (int i = startIdx; i <= endIdx; ++i)
    {
        const double g = localGradientPct[static_cast<size_t>(i)];
        if (isFinite(g))
            climbGradients.push_back(g);
    }

    climb.category = classifyClimbCategory(climb.elevationGainM, climb.difficultyScore);
    climb.shapeClass = classifyClimbShape(
        climbGradients,
        climb.lengthKm,
        climb.elevationGainM,
        climb.averageGradient,
        climb.maximumGradient);
    climb.name = QString("Climb %1").arg(climbOrdinal);

    return climb;
}