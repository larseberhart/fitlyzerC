// SPDX-License-Identifier: GPL-3

#include "WorkoutPersistenceService.h"

#include "database/DatabaseManager.h"
#include "model/RideDataSerializer.h"

RideData WorkoutPersistenceService::loadRide(int activityId, DatabaseManager& dbManager)
{
    return RideDataSerializer::loadRideFromDatabase(activityId, dbManager);
}

int WorkoutPersistenceService::saveRide(const RideData& ride,
                                        const WorkoutStatistics& stats,
                                        int athleteId,
                                        const QString& sourcePath,
                                        const QString& fitHash,
                                        double normalizedPower,
                                        DatabaseManager& dbManager,
                                        ActivityRepository& activityRepository,
                                        ImportRepository& importRepository,
                                        QString* errorOut)
{
    return RideDataSerializer::saveRideToDatabase(
        ride,
        stats,
        athleteId,
        sourcePath,
        fitHash,
        normalizedPower,
        dbManager,
        activityRepository,
        importRepository,
        errorOut);
}
