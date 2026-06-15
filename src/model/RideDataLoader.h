#pragma once

#include "fit/RideData.h"
#include <QSqlDatabase>

/**
 * Centralized utility for loading activity samples from the database.
 * Eliminates code duplication between RideDataSerializer and AnalysisWorker.
 */
class RideDataLoader
{
public:
    /**
     * Load all samples for an activity from the database.
     *
     * @param activityId The activity ID to load samples for
     * @param db The database connection to use
     * @return RideData with samples; empty if none found or on error
     *
     * Optimizations:
     * - Pre-reserves vector using COUNT(*) to avoid repeated allocations
     * - Sets forward-only mode on query for reduced memory overhead
     */
    static RideData loadActivitySamples(int activityId, QSqlDatabase& db);
};
