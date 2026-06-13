#pragma once

#include "fit/RideData.h"

struct TrainingMetricsSummary
{
    double normalizedPower = 0.0;
    double intensityFactor = 0.0;
    double trainingStressScore = 0.0;
    double variabilityIndex = 0.0;
    double efficiencyFactor = 0.0;
    double averagePower = 0.0;
    double averageHeartRate = 0.0;
};

class TrainingMetrics
{
public:
    static TrainingMetricsSummary compute(const RideData& rideData,
                                          double ftp,
                                          double durationSeconds = 0.0);
};
