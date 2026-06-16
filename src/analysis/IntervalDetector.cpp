// SPDX-License-Identifier: GPL-3


#include "IntervalDetector.h"

#include <algorithm>
#include <cmath>

std::vector<double> IntervalDetector::smoothPower(
    const RideData& rideData, int window)
{
    const int n = static_cast<int>(rideData.records.size());
    std::vector<double> raw(n);
    for (int i = 0; i < n; ++i)
    {
        const auto& r = rideData.records[i];
        raw[i] = r.hasPower ? r.power : 0.0;
    }

    const int half = window / 2;
    std::vector<double> out(n);
    for (int i = 0; i < n; ++i)
    {
        const int lo = std::max(0, i - half);
        const int hi = std::min(n - 1, i + half);
        double s = 0.0;
        for (int j = lo; j <= hi; ++j) s += raw[j];
        out[i] = s / (hi - lo + 1);
    }

    return out;
}

Interval IntervalDetector::buildInterval(
    const RideData& rideData,
    int startIdx, int endIdx,
    const std::string& label)
{
    Interval iv;
    iv.label           = label;
    iv.startSeconds    = rideData.records[startIdx].elapsedSeconds;
    iv.endSeconds      = rideData.records[endIdx].elapsedSeconds;
    iv.durationSeconds = iv.endSeconds - iv.startSeconds;

    std::vector<double> pArr;
    double pSum = 0;  int pN = 0;
    double hrSum = 0; int hrN = 0;
    double cadSum = 0; int cadN = 0;
    double spdSum = 0; int spdN = 0;

    for (int i = startIdx; i <= endIdx; ++i)
    {
        const auto& r = rideData.records[i];
        pArr.push_back(r.hasPower ? r.power : 0.0);

        if (r.hasPower)
        {
            pSum += r.power;
            iv.maximumPower = std::max(iv.maximumPower, r.power);
            ++pN;
        }
        if (r.hasHeartRate) { hrSum  += r.heartRate; ++hrN;  }
        if (r.hasCadence)   { cadSum += r.cadence;   ++cadN; }
        if (r.hasSpeed)     { spdSum += r.speed;     ++spdN; }
    }

    if (pN   > 0) iv.averagePower     = pSum  / pN;
    if (hrN  > 0) iv.averageHeartRate = hrSum  / hrN;
    if (cadN > 0) iv.averageCadence   = cadSum / cadN;
    if (spdN > 0) iv.averageSpeed     = spdSum / spdN;

    // Normalised Power (30 s rolling)
    const int npWin = std::min(30, static_cast<int>(pArr.size()));
    if (npWin > 0)
    {
        double wSum = 0.0;
        for (int i = 0; i < npWin; ++i) wSum += pArr[i];
        double s4 = std::pow(wSum / npWin, 4.0);
        int cnt = 1;
        for (int i = npWin; i < static_cast<int>(pArr.size()); ++i)
        {
            wSum += pArr[i] - pArr[i - npWin];
            s4   += std::pow(wSum / npWin, 4.0);
            ++cnt;
        }
        iv.normalizedPower = std::pow(s4 / cnt, 0.25);
    }

    return iv;
}

std::vector<Interval> IntervalDetector::detect(
    const RideData& rideData, const Config& config)
{
    const auto& records = rideData.records;
    if (records.empty()) return {};

    bool hasPower = false;
    for (const auto& r : records)
        if (r.hasPower) { hasPower = true; break; }
    if (!hasPower) return {};

    const double threshold =
        config.ftp * config.workThresholdPct;
    const auto smoothed =
        smoothPower(rideData, config.smoothingWindow);

    const int n = static_cast<int>(records.size());
    std::vector<Interval> result;

    int  segStart = 0;
    bool inWork   = smoothed[0] >= threshold;

    auto flush = [&](int endIdx)
    {
        if (endIdx < segStart) return;
        const double dur =
            records[endIdx].elapsedSeconds -
            records[segStart].elapsedSeconds;

        if (inWork && dur >= config.minWorkSeconds)
            result.push_back(
                buildInterval(rideData, segStart, endIdx, "Work"));
        else if (!inWork &&
                 dur >= config.minRecoverySeconds &&
                 !result.empty())
            result.push_back(
                buildInterval(rideData, segStart, endIdx, "Recovery"));
    };

    for (int i = 1; i < n; ++i)
    {
        const bool nowWork = smoothed[i] >= threshold;
        if (nowWork != inWork)
        {
            flush(i - 1);
            segStart = i;
            inWork   = nowWork;
        }
    }
    flush(n - 1);

    return result;
}

std::vector<Interval> IntervalDetector::removeOverlappingAndDuplicates(
    std::vector<Interval> intervals)
{
    if (intervals.size() <= 1)
        return intervals;

    // Sort by start time
    std::sort(intervals.begin(), intervals.end(),
        [](const Interval& a, const Interval& b) {
            return a.startSeconds < b.startSeconds;
        });

    std::vector<Interval> result;
    result.reserve(intervals.size());

    // Merge overlapping/adjacent intervals and remove duplicates
    const double tolerance = 1.0;  // 1 second boundary tolerance for duplicates

    for (const Interval& current : intervals)
    {
        // Skip if this interval is a duplicate of the last one
        // (same start/end within tolerance)
        if (!result.empty())
        {
            const Interval& last = result.back();
            if (std::abs(last.startSeconds - current.startSeconds) <= tolerance &&
                std::abs(last.endSeconds - current.endSeconds) <= tolerance)
            {
                // Duplicate detected: keep the existing one
                continue;
            }

            // Check for overlap: if current starts before last ends, merge them
            if (current.startSeconds < last.endSeconds + tolerance)
            {
                // Merge by extending the end time and averaging metrics
                Interval& merged = result.back();
                merged.endSeconds = std::max(merged.endSeconds, current.endSeconds);
                merged.durationSeconds = merged.endSeconds - merged.startSeconds;

                // Average the metrics proportionally (weighted by interval duration)
                const double lastDur = last.durationSeconds;
                const double currDur = current.durationSeconds;
                const double totalDur = lastDur + currDur;

                if (totalDur > 0.0)
                {
                    merged.averagePower     = (last.averagePower * lastDur + current.averagePower * currDur) / totalDur;
                    merged.maximumPower     = std::max(last.maximumPower, current.maximumPower);
                    merged.normalizedPower  = (last.normalizedPower * lastDur + current.normalizedPower * currDur) / totalDur;
                    merged.averageHeartRate = (last.averageHeartRate * lastDur + current.averageHeartRate * currDur) / totalDur;
                    merged.averageCadence   = (last.averageCadence * lastDur + current.averageCadence * currDur) / totalDur;
                    merged.averageSpeed     = (last.averageSpeed * lastDur + current.averageSpeed * currDur) / totalDur;
                }
                continue;
            }
        }

        // No overlap/duplicate: add as new interval
        result.push_back(current);
    }

    return result;
}