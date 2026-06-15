// SPDX-License-Identifier: GPL-3

/**
 * @file RideDataLoader.cpp
 * @brief Data model support for RideDataLoader.
 *
 * Defines model loading, serialization, and model-related helpers for ride and activity data handling.
 *
 * Responsibilities:
 * - Provide model serialization or data-loading behavior
 *
 * @author Lars EBERHART
 */

#include "RideDataLoader.h"

#include <QSqlQuery>

RideData RideDataLoader::loadActivitySamples(int activityId, QSqlDatabase& db)
{
    RideData ride;

    // Pre-reserve vector to avoid repeated reallocations during large sample loads
    {
        QSqlQuery countQ(db);
        countQ.prepare("SELECT COUNT(*) FROM activity_samples WHERE activity_id=:id");
        countQ.bindValue(":id", activityId);
        if (countQ.exec() && countQ.next())
        {
            const int sampleCount = countQ.value(0).toInt();
            if (sampleCount > 0)
                ride.records.reserve(sampleCount);
        }
    }

    // Load all samples with forward-only mode for memory efficiency
    QSqlQuery samplesQuery(db);
    samplesQuery.prepare(
        "SELECT elapsed_seconds, power_total, heart_rate, cadence, speed,"
        "  latitude, longitude, altitude,"
        "  has_gps, has_power, has_heart_rate, has_cadence, has_speed, has_altitude"
        " FROM activity_samples WHERE activity_id=:id ORDER BY elapsed_seconds ASC");
    samplesQuery.bindValue(":id", activityId);
    samplesQuery.setForwardOnly(true);

    if (samplesQuery.exec())
    {
        while (samplesQuery.next())
        {
            RideRecord rec;
            rec.elapsedSeconds = samplesQuery.value(0).toDouble();
            rec.power          = samplesQuery.value(1).toDouble();
            rec.heartRate      = samplesQuery.value(2).toDouble();
            rec.cadence        = samplesQuery.value(3).toDouble();
            rec.speed          = samplesQuery.value(4).toDouble();
            rec.latitude       = samplesQuery.value(5).toDouble();
            rec.longitude      = samplesQuery.value(6).toDouble();
            rec.altitude       = samplesQuery.value(7).toDouble();
            rec.hasGps         = samplesQuery.value(8).toInt() != 0;
            rec.hasPower       = samplesQuery.value(9).toInt() != 0;
            rec.hasHeartRate   = samplesQuery.value(10).toInt() != 0;
            rec.hasCadence     = samplesQuery.value(11).toInt() != 0;
            rec.hasSpeed       = samplesQuery.value(12).toInt() != 0;
            rec.hasAltitude    = samplesQuery.value(13).toInt() != 0;
            ride.records.push_back(rec);
        }
    }

    return ride;
}