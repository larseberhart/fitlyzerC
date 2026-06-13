#include "WorkoutAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <numbers>

static double segmentKm(double lat1, double lon1, double lat2, double lon2)
{
    constexpr double R = 6371.0;
    const double dLat = (lat2 - lat1) * std::numbers::pi_v<double> / 180.0;
    const double dLon = (lon2 - lon1) * std::numbers::pi_v<double> / 180.0;
    const double mLat = (lat1 + lat2) / 2.0 * std::numbers::pi_v<double> / 180.0;
    return std::hypot(dLat, dLon * std::cos(mLat)) * R;
}

WorkoutStatistics WorkoutAnalyzer::analyze(const RideData& rideData)
{
    WorkoutStatistics stats;
    if (rideData.records.empty()) return stats;

    const auto& records = rideData.records;
    const int   n       = static_cast<int>(records.size());

    double powerSum = 0, hrSum = 0, cadSum = 0, spdSum = 0;
    int    powerN   = 0, hrN   = 0, cadN   = 0, spdN   = 0;

    bool   prevHasGps = false;
    double prevLat = 0, prevLon = 0;
    bool   usedGps = false;

    bool   prevHasAlt = false;
    double prevAlt    = 0;

    for (int i = 0; i < n; ++i)
    {
        const auto& r = records[i];
        stats.durationSeconds = r.elapsedSeconds;

        if (r.hasPower)
        {
            powerSum += r.power;
            stats.maximumPower = std::max(stats.maximumPower, r.power);
            ++powerN;
        }

        if (r.hasHeartRate)
        {
            hrSum += r.heartRate;
            stats.maximumHeartRate = std::max(stats.maximumHeartRate, r.heartRate);
            ++hrN;
        }

        if (r.hasCadence)
        {
            cadSum += r.cadence;
            stats.maximumCadence = std::max(stats.maximumCadence, r.cadence);
            ++cadN;
        }

        if (r.hasSpeed)
        {
            spdSum += r.speed;
            stats.maximumSpeed = std::max(stats.maximumSpeed, r.speed);
            ++spdN;

            if (r.speed > 2.0 && i > 0)
            {
                const double dt = r.elapsedSeconds - records[i-1].elapsedSeconds;
                stats.movingTimeSeconds += std::max(0.0, dt);
            }
        }

        if (r.hasGps)
        {
            if (prevHasGps)
            {
                stats.totalDistanceKm +=
                    segmentKm(prevLat, prevLon, r.latitude, r.longitude);
                usedGps = true;
            }
            prevHasGps = true;
            prevLat    = r.latitude;
            prevLon    = r.longitude;
        }

        if (r.hasAltitude)
        {
            if (prevHasAlt && r.altitude > prevAlt)
                stats.elevationGainM += r.altitude - prevAlt;
            prevHasAlt = true;
            prevAlt    = r.altitude;
        }
    }

    if (!usedGps && spdN > 0)
    {
        for (int i = 1; i < n; ++i)
        {
            const auto& r    = records[i];
            const auto& prev = records[i-1];
            if (!r.hasSpeed) continue;
            const double dt  = r.elapsedSeconds - prev.elapsedSeconds;
            const double spd = (r.speed + (prev.hasSpeed ? prev.speed : r.speed)) / 2.0;
            stats.totalDistanceKm += spd * std::max(0.0, dt) / 3600.0;
        }
    }

    if (powerN) stats.averagePower     = powerSum / powerN;
    if (hrN)    stats.averageHeartRate = hrSum    / hrN;
    if (cadN)   stats.averageCadence   = cadSum   / cadN;
    if (spdN)   stats.averageSpeed     = spdSum   / spdN;

    return stats;
}
