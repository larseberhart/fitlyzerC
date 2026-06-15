// SPDX-License-Identifier: GPL-3

/**
 * @file RideDataSerializer.h
 * @brief Data model support for RideDataSerializer.
 *
 * Defines model loading, serialization, and model-related helpers for ride and activity data handling.
 *
 * Responsibilities:
 * - Provide model serialization or data-loading behavior
 *
 * @author Lars EBERHART
 */

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
    /**
     * @brief Saves parsed ride to database.
     * @param rideData Ride sample data.
     * @param stats Computed statistics.
     * @param athleteId Athlete ID.
     * @param filePath Original file path.
     * @param fitHash SHA-256 hash of FIT file.
     * @param normalizedPower Computed normalized power.
     * @param dbManager Database manager.
     * @param activityRepo Activity repository.
     * @param importRepo Import log repository.
     * @param errorOut Optional error message output.
     * @return New activity ID on success, -1 on error.
     */
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

    /**
     * @brief Loads ride data from the database.
     * @param activityId Activity ID to load.
     * @param dbManager Database manager.
     * @return RideData with samples from database.
     */
    static RideData loadRideFromDatabase(int activityId, DatabaseManager& dbManager);
};