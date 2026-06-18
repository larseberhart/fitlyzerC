// SPDX-License-Identifier: GPL-3

#include "ClimbMerger.h"

#include <algorithm>
#include <cmath>

namespace ClimbMerger
{
std::vector<Climb> merge(std::vector<Climb>& candidates,
                         const ClimbDetector::Config& config)
{
    if (!config.enableClimbMerging || candidates.size() < 2)
        return candidates;

    std::vector<Climb> merged;
    std::vector<bool> processed(candidates.size(), false);

    for (size_t i = 0; i < candidates.size(); ++i)
    {
        if (processed[i])
            continue;

        Climb combined = candidates[i];
        processed[i] = true;

        size_t nextIdx = i + 1;
        while (nextIdx < candidates.size())
        {
            if (processed[nextIdx])
            {
                ++nextIdx;
                continue;
            }

            const Climb& next = candidates[nextIdx];

            const double gapMeters = next.startDistanceKm * 1000.0 - combined.endDistanceKm * 1000.0;
            const double elevationGapM = std::abs(next.startElevationM - combined.endElevationM);
            const bool overlaps = gapMeters <= 0.0;

            if (overlaps ||
                gapMeters <= config.mergeMaxGapMeters ||
                elevationGapM <= config.mergeMaxGapElevationM)
            {
                const double bridgingGainM = std::max(0.0, next.startElevationM - combined.endElevationM);
                combined.endSeconds = std::max(combined.endSeconds, next.endSeconds);
                combined.endDistanceKm = std::max(combined.endDistanceKm, next.endDistanceKm);
                combined.durationSeconds = combined.endSeconds - combined.startSeconds;
                combined.lengthKm = combined.endDistanceKm - combined.startDistanceKm;
                if (next.endDistanceKm >= combined.endDistanceKm)
                    combined.endElevationM = next.endElevationM;

                combined.elevationGainM += overlaps ? next.elevationGainM : bridgingGainM + next.elevationGainM;

                if (combined.lengthKm > 0.0)
                    combined.averageGradient = (combined.elevationGainM / (combined.lengthKm * 1000.0)) * 100.0;

                processed[nextIdx] = true;
                ++nextIdx;
            }
            else
            {
                break;
            }
        }

        merged.push_back(combined);
    }

    return merged;
}
}
