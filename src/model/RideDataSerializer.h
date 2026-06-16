// SPDX-License-Identifier: GPL-3


#pragma once

#include <QString>

#include "database/ActivityRepository.h"
#include "database/ImportRepository.h"
#include "analysis/WorkoutAnalyzer.h"
#include "fit/RideData.h"

class DatabaseManager;

/**
 * @brief Persists ride data to database.
 *
 * Saves parsed rides with computed statistics, detects duplicates,
 * and logs all import attempts.
 */
class RideDataSerializer
{
public:
    static int saveRideToDatabase(
        const RideData&           rideData,
        const WorkoutStatistics&  stats,
        int                       athleteId,
        const QString&            filePath,
        const QString&            fitHash,   // SHA-256 hex of the .fit file bytes
        double                    normalizedPower,
        DatabaseManager&          dbManager,
        ActivityRepository&       activityRepo,
        ImportRepository&         importRepo,
        QString*                  errorOut = nullptr);

    static RideData loadRideFromDatabase(int activityId, DatabaseManager& dbManager);
};