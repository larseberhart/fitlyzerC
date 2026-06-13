#include "TrainingLoad.h"

#include <cmath>
#include <vector>

double TrainingLoad::normalizedPower(const RideData& rideData)
{
    const auto& records = rideData.records;
    if (records.empty()) return 0.0;

    std::vector<double> power;
    power.reserve(records.size());
    for (const auto& r : records)
        power.push_back(r.hasPower ? r.power : 0.0);

    constexpr int kWindow = 30;
    const int n = static_cast<int>(power.size());
    if (n < kWindow) return 0.0;

    double windowSum = 0.0;
    for (int i = 0; i < kWindow; ++i)
        windowSum += power[i];

    double sum4 = std::pow(windowSum / kWindow, 4.0);
    int    count = 1;

    for (int i = kWindow; i < n; ++i)
    {
        windowSum += power[i] - power[i - kWindow];
        sum4      += std::pow(windowSum / kWindow, 4.0);
        ++count;
    }

    return std::pow(sum4 / count, 0.25);
}

double TrainingLoad::intensityFactor(double np, double ftp)
{
    if (ftp <= 0.0) return 0.0;
    return np / ftp;
}

double TrainingLoad::trainingStressScore(
    double durationSeconds, double np, double ftp)
{
    if (ftp <= 0.0) return 0.0;
    const double IF = intensityFactor(np, ftp);
    return (durationSeconds * np * IF) / (ftp * 3600.0) * 100.0;
}

std::vector<FitnessMetricsPoint> TrainingLoad::fitnessTimeline(
    const std::vector<DailyLoadPoint>& dailyLoads,
    int atlDays,
    int ctlDays)
{
    std::vector<FitnessMetricsPoint> out;
    out.reserve(dailyLoads.size());

    if (dailyLoads.empty())
        return out;

    const double atlAlpha = atlDays > 0 ? (2.0 / (atlDays + 1.0)) : 1.0;
    const double ctlAlpha = ctlDays > 0 ? (2.0 / (ctlDays + 1.0)) : 1.0;

    double atl = 0.0;
    double ctl = 0.0;
    bool initialized = false;

    for (const DailyLoadPoint& p : dailyLoads)
    {
        if (!initialized)
        {
            atl = p.tss;
            ctl = p.tss;
            initialized = true;
        }
        else
        {
            atl += atlAlpha * (p.tss - atl);
            ctl += ctlAlpha * (p.tss - ctl);
        }

        FitnessMetricsPoint point;
        point.atl = atl;
        point.ctl = ctl;
        point.tsb = ctl - atl;
        out.push_back(point);
    }

    return out;
}
