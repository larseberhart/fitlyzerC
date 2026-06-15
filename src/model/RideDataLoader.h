// SPDX-License-Identifier: GPL-3

/**
 * @file RideDataLoader.h
 * @brief Data model support for RideDataLoader.
 *
 * Defines model loading, serialization, and model-related helpers for ride and activity data handling.
 *
 * Responsibilities:
 * - Provide model serialization or data-loading behavior
 *
 * @author Lars EBERHART
 */

#pragma once

#include "fit/RideData.h"
#include <QSqlDatabase>

/**
 * @brief Loads activity ride data samples from the database.
 */
class RideDataLoader
{
public:
    /**
     * @brief Loads all ride samples for an activity from database.
     * @param activityId Activity ID.
     * @param db Database connection.
     * @return Ride data; empty if not found or error.
     */
    static RideData loadActivitySamples(int activityId, QSqlDatabase& db);
};