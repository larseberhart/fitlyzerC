// SPDX-License-Identifier: GPL-3

#include "ClimbClassifier.h"

namespace ClimbClassifier
{
ClimbCategory categorize(double gainMeters,
                         double lengthKm,
                         double averageGrade)
{
    if (gainMeters > 100.0 && lengthKm > 2.0 && averageGrade > 3.0)
        return ClimbCategory::Major;

    if (gainMeters > 40.0 && lengthKm > 0.5 && averageGrade > 3.0)
        return ClimbCategory::Medium;

    if (gainMeters > 10.0 && lengthKm > 0.1 && averageGrade > 5.0)
        return ClimbCategory::Punchy;

    return ClimbCategory::FalsePositive;
}
}
