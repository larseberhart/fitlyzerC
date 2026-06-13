#include "FTPZones.h"

#include <algorithm>
#include <cmath>

FTPZones::ZoneArray FTPZones::defaultZones()
{
    return {{
        { 1, "Active Recovery", 0.00,  0.55, 0.0 },
        { 2, "Endurance",       0.55,  0.75, 0.0 },
        { 3, "Tempo",           0.75,  0.90, 0.0 },
        { 4, "Threshold",       0.90,  1.05, 0.0 },
        { 5, "VO2 Max",         1.05,  1.20, 0.0 },
        { 6, "Anaerobic",       1.20,  1.50, 0.0 },
        { 7, "Neuromuscular",   1.50, -1.00, 0.0 },
    }};
}

FTPZones::ZoneArray FTPZones::compute(
    const RideData& rideData, double ftp)
{
    auto zones = defaultZones();
    if (ftp <= 0.0 || rideData.records.empty())
        return zones;

    const auto& records = rideData.records;
    const int n = static_cast<int>(records.size());

    for (int i = 0; i < n; ++i)
    {
        const auto& r = records[i];
        if (!r.hasPower) continue;

        // Midpoint time weight
        double dt = 1.0;
        if (n > 1)
        {
            if (i == 0)
                dt = records[1].elapsedSeconds - records[0].elapsedSeconds;
            else if (i == n - 1)
                dt = records[n - 1].elapsedSeconds - records[n - 2].elapsedSeconds;
            else
                dt = (records[i + 1].elapsedSeconds -
                      records[i - 1].elapsedSeconds) / 2.0;
        }
        dt = std::max(0.0, dt);

        const double pct = r.power / ftp;
        for (auto& zone : zones)
        {
            if (pct >= zone.lowerPct &&
                (zone.upperPct < 0.0 || pct < zone.upperPct))
            {
                zone.seconds += dt;
                break;
            }
        }
    }

    return zones;
}
