// SPDX-License-Identifier: GPL-3

#pragma once

#include <vector>

#include "analysis/ClimbDetector.h"

namespace ClimbMerger
{
std::vector<Climb> merge(std::vector<Climb>& candidates,
                         const ClimbDetector::Config& config);
}
