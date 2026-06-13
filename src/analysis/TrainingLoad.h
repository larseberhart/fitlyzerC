#pragma once

#include <vector>

#include "fit/RideData.h"

struct DailyLoadPoint
{
    double tss = 0.0;
};

struct FitnessMetricsPoint
{
    double atl = 0.0; // 7-day EWMA
    double ctl = 0.0; // 42-day EWMA
    double tsb = 0.0; // ctl - atl
};

class TrainingLoad
{
public:
    // Normalised Power: 30 s rolling mean raised to the 4th power, then ^0.25
    static double normalizedPower(const RideData& rideData);

    // Intensity Factor = NP / FTP
    static double intensityFactor(double np, double ftp);

    // Training Stress Score = (sec × NP × IF) / (FTP × 3600) × 100
    static double trainingStressScore(
        double durationSeconds, double np, double ftp);

    // Computes ATL/CTL/TSB sequence from daily TSS values.
    static std::vector<FitnessMetricsPoint> fitnessTimeline(
        const std::vector<DailyLoadPoint>& dailyLoads,
        int atlDays = 7,
        int ctlDays = 42);
};
