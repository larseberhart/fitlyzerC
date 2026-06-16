// SPDX-License-Identifier: GPL-3


#pragma once

#include <QString>

#include "RideData.h"

/**
 * @brief Parses FIT files and extracts activity recording data.
 */
class FitParser
{
public:
    /**
     * @brief Loads activity data from a FIT file.
     * @param fileName Path to .fit file.
     * @return RideData with parsed activity records.
     */
    RideData load(const QString& fileName);

private:
    static double semicirclesToDegrees(long value);
};