// SPDX-License-Identifier: GPL-3

#pragma once

#include <vector>

#include "fit/RideData.h"

namespace ClimbCandidateFinder
{
std::vector<double> buildCumulativeDistanceMeters(const RideData& rideData);
}
