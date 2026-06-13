#include "TrainingMetrics.h"

#include "analysis/TrainingLoad.h"

TrainingMetricsSummary TrainingMetrics::compute(const RideData& rideData,
                                                double ftp,
                                                double durationSeconds)
{
    TrainingMetricsSummary out;

    if (durationSeconds <= 0.0 && !rideData.records.empty())
        durationSeconds = rideData.records.back().elapsedSeconds;

    double powerSum = 0.0;
    int powerCount = 0;
    double hrSum = 0.0;
    int hrCount = 0;

    for (const RideRecord& r : rideData.records)
    {
        if (r.hasPower && r.power > 0.0)
        {
            powerSum += r.power;
            ++powerCount;
        }
        if (r.hasHeartRate && r.heartRate > 0.0)
        {
            hrSum += r.heartRate;
            ++hrCount;
        }
    }

    out.averagePower = powerCount > 0 ? (powerSum / powerCount) : 0.0;
    out.averageHeartRate = hrCount > 0 ? (hrSum / hrCount) : 0.0;

    out.normalizedPower = TrainingLoad::normalizedPower(rideData);
    out.intensityFactor = TrainingLoad::intensityFactor(out.normalizedPower, ftp);
    out.trainingStressScore = TrainingLoad::trainingStressScore(durationSeconds, out.normalizedPower, ftp);

    if (out.averagePower > 0.0)
        out.variabilityIndex = out.normalizedPower / out.averagePower;

    if (out.averageHeartRate > 0.0)
        out.efficiencyFactor = out.normalizedPower / out.averageHeartRate;

    return out;
}
