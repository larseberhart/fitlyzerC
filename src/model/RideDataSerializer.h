#pragma once

#include <QString>

#include "database/ActivityRepository.h"
#include "database/ImportRepository.h"
#include "analysis/WorkoutAnalyzer.h"
#include "fit/RideData.h"

class DatabaseManager;

class RideDataSerializer
{
public:
    // Save a parsed ride to the database.
    // Returns the new activity id, or -1 on error.
    // Logs every attempt (success, duplicate, error) to ImportRepository.
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

    // Load a ride from the database back into a RideData object.
    static RideData loadRideFromDatabase(int activityId, DatabaseManager& dbManager);
};
