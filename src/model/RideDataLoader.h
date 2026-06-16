// SPDX-License-Identifier: GPL-3


#pragma once

#include "fit/RideData.h"
#include <QSqlDatabase>

/**
 * @brief Loads activity ride data samples from the database.
 */
class RideDataLoader
{
public:
    static RideData loadActivitySamples(int activityId, QSqlDatabase& db);
};