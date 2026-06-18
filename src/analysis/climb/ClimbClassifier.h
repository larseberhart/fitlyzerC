// SPDX-License-Identifier: GPL-3

#pragma once

#include "analysis/ClimbMetrics.h"

namespace ClimbClassifier
{
ClimbCategory categorize(double gainMeters,
                         double lengthKm,
                         double averageGrade);
}
