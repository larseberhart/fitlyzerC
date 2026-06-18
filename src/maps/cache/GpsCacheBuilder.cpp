// SPDX-License-Identifier: GPL-3

#include "GpsCacheBuilder.h"

namespace GpsCacheBuilder
{
GpsCacheData build(const RideData& rideData)
{
    GpsCacheData cache;
    cache.records.reserve(rideData.records.size());

    for (const auto& r : rideData.records)
    {
        if (!r.hasGps)
            continue;

        cache.records.push_back(&r);
        if (!cache.firstRecord)
            cache.firstRecord = &r;
        cache.lastRecord = &r;
    }

    return cache;
}
}
