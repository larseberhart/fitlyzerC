// SPDX-License-Identifier: GPL-3


#pragma once

#include <QList>
#include <QString>

class QSqlDatabase;

/**
 * @brief Represents a single recorded activity or workout.
 *
 * Contains all activity metadata, measurements, and athlete information.
 */
struct Activity
{
    /// @brief Unique activity identifier.
    int     id              = -1;

    /// @brief Associated athlete ID.
    int     athleteId       = -1;

    /// @brief SHA-256 hash of original FIT file for duplicate detection.
    QString fitHash;

    /// @brief Original filename.
    QString fileName;

    /// @brief Sport type (cycling, running, etc.).
    QString sport;

    /// @brief ISO8601 activity start time.
    QString startTime;

    /// @brief ISO8601 activity end time.
    QString endTime;

    /// @brief Total duration in seconds.
    double  durationSec     = 0.0;

    /// @brief Total distance in meters.
    double  distanceM       = 0.0;

    /// @brief Average power in watts.
    double  avgPower        = 0.0;

    /// @brief Maximum power in watts.
    double  maxPower        = 0.0;

    /// @brief Normalized power in watts.
    double  normalizedPower = 0.0;

    /// @brief Average heart rate in bpm.
    double  avgHr           = 0.0;

    /// @brief Maximum heart rate in bpm.
    double  maxHr           = 0.0;

    /// @brief Average cadence in rpm.
    double  avgCadence      = 0.0;

    /// @brief Average speed in km/h.
    double  avgSpeed        = 0.0;

    /// @brief Total elevation gain in meters.
    double  elevationGain   = 0.0;

    /// @brief User activity notes.
    QString notes;

    /// @brief Weather/environmental notes.
    QString weatherNotes;

    /// @brief Temperature in Celsius.
    double  temperature     = 0.0;

    /// @brief Weather description.
    QString weather;

    /// @brief Wind conditions.
    QString wind;

    /// @brief Rate of perceived exertion (1-10).
    int     rpe             = 0;

    /// @brief Fatigue level (1-10).
    int     fatigue         = 0;

    /// @brief Sleep before activity in hours.
    double  sleepHours      = 0.0;

    /// @brief Athlete weight in kg.
    double  weightKg        = 0.0;

    /// @brief Bike used.
    QString bike;

    /// @brief Equipment used.
    QString equipment;

    /// @brief ISO8601 import timestamp.
    QString importedAt;

    /// @brief Analysis algorithm version used.
    int     analysisVersion = 1;

    /// @brief Content hash for detecting meaningful changes.
    QString fingerprint;

    /// @brief Bitmask of analysis flags (climbs, intervals, etc.).
    int     analysisFlags   = 0;

    /// @brief Source of import (file, web, api, etc.).
    QString importSource;

    /**
     * @brief Checks if activity data is valid.
     * @return True if activity has a valid positive ID.
     */
    bool isValid() const { return id > 0; }
};

/**
 * @brief Repository for activity record persistence.
 *
 * Provides CRUD operations and queries for activities in the database.
 */
class ActivityRepository
{
public:
    /**
     * @brief Constructs repository with database connection.
     * @param db Database connection.
     */
    explicit ActivityRepository(QSqlDatabase& db);

    /**
     * @brief Inserts a new activity into the database.
     * @param a Activity to insert.
     * @return ID of inserted activity, or -1 on error.
     */
    int            insertActivity(const Activity& a);

    /**
     * @brief Retrieves an activity by ID.
     * @param activityId Activity ID.
     * @return Activity record, or invalid Activity if not found.
     */
    Activity       getActivity(int activityId);

    /**
     * @brief Lists all activities for an athlete.
     * @param athleteId Athlete ID.
     * @return List of activities.
     */
    QList<Activity> listActivities(int athleteId);

    /**
     * @brief Deletes an activity.
     * @param activityId Activity ID to delete.
     * @return True if deletion succeeded.
     */
    bool           deleteActivity(int activityId);

    /**
     * @brief Checks if activity exists by FIT file hash.
     * @param hash SHA-256 hash of FIT file.
     * @return True if hash already exists in database.
     */
    bool           hasFitHash(const QString& hash);

    /**
     * @brief Updates activity notes and weather information.
     * @param activityId Activity ID.
     * @param notes Activity notes.
     * @param weatherNotes Weather observations.
     * @return True if update succeeded.
     */
    bool           updateNotes(int activityId, const QString& notes,
                               const QString& weatherNotes);

    /**
     * @brief Updates comprehensive activity properties.
     * @param activityId Activity ID.
     * @param a Activity with new values.
     * @return True if update succeeded.
     */
    bool           updateActivityProperties(int activityId, const Activity& a);

    /**
     * @brief Updates analysis flags for an activity.
     * @param activityId Activity ID.
     * @param flags Analysis flag bitmask.
     * @return True if update succeeded.
     */
    bool           updateAnalysisFlags(int activityId, int flags);

    /**
     * @brief Updates activity fingerprint for duplicate detection.
     * @param activityId Activity ID.
     * @param fingerprint Fingerprint string.
     * @return True if update succeeded.
     */
    bool           updateFingerprint(int activityId, const QString& fingerprint);

    /**
     * @brief Checks if fingerprint already exists.
     * @param fingerprint Fingerprint to check.
     * @return True if match found.
     */
    bool           hasFingerprintMatch(const QString& fingerprint);

    /**
     * @brief Sets the import source label for an activity.
     * @param activityId Activity ID.
     * @param source Import source label (e.g., "manual_import", "directory_import").
     * @return True if update succeeded.
     */
    bool           setImportSource(int activityId, const QString& source);

private:
    QSqlDatabase& m_db;
};