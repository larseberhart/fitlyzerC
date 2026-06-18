// SPDX-License-Identifier: GPL-3

#pragma once

#include <vector>

#include "fit/RideData.h"

struct GpsCacheData
{
    std::vector<const RideRecord*> records;
    const RideRecord* firstRecord = nullptr;
    const RideRecord* lastRecord = nullptr;
};

namespace GpsCacheBuilder
{
GpsCacheData build(const RideData& rideData);
}
