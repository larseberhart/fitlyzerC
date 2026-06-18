// SPDX-License-Identifier: GPL-3

#pragma once

#include <QString>

#include "analysis/WorkoutAnalyzer.h"
#include "database/ActivityRepository.h"
#include "database/ImportRepository.h"
#include "fit/RideData.h"

class DatabaseManager;

class WorkoutPersistenceService
{
public:
    static RideData loadRide(int activityId, DatabaseManager& dbManager);

    static int saveRide(const RideData& ride,
                        const WorkoutStatistics& stats,
                        int athleteId,
                        const QString& sourcePath,
                        const QString& fitHash,
                        double normalizedPower,
                        DatabaseManager& dbManager,
                        ActivityRepository& activityRepository,
                        ImportRepository& importRepository,
                        QString* errorOut);
};
