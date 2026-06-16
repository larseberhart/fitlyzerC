// SPDX-License-Identifier: GPL-3


#pragma once

#include <QList>
#include <QString>

class QSqlDatabase;

/**
 * @brief Database representation of a stored climb.
 *
 * Contains all climb metrics and lifecycle information persisted in the database.
 */
struct ClimbRecord
{
    /// @brief Unique climb record ID.
    int     id                    = -1;

    /// @brief Associated activity ID.
    int     activityId            = -1;

    /// @brief Stable UUID, generated once on insert.
    QString uuid;

    /// @brief Climb start time in seconds from activity start.
    double  startSeconds          = 0.0;

    /// @brief Climb end time in seconds from activity start.
    double  endSeconds            = 0.0;

    /// @brief Start bounds at first auto-detection (-1 if not set).
    double  originalStartSeconds  = -1.0;

    /// @brief End bounds at first auto-detection (-1 if not set).
    double  originalEndSeconds    = -1.0;

    /// @brief Climb name.
    QString name;

    /// @brief Elevation gain in meters.
    double  elevationGainM   = 0.0;

    /// @brief Climb length in kilometers.
    double  lengthKm         = 0.0;

    /// @brief Average gradient percentage.
    double  averageGradient  = 0.0;

    /// @brief Average power in watts (cached).
    double  avgPower         = 0.0;

    /// @brief Normalized power in watts (cached).
    double  np               = 0.0;

    /// @brief Average heart rate in bpm (cached).
    double  avgHr            = 0.0;

    /// @brief Average cadence in rpm (cached).
    double  avgCadence       = 0.0;

    /// @brief VAM (Vertical Ascent Meter) in m/h (cached).
    double  vam              = 0.0;

    /// @brief Source of climb (auto, manual, edited).
    QString source           = QStringLiteral("auto");

    /// @brief Whether climb is locked from editing.
    bool    locked           = false;

    /// @brief Soft-delete flag.
    bool    deleted          = false;

    /// @brief Algorithm version used for detection.
    int     algorithmVersion = 1;

    /// @brief User notes about climb.
    QString notes;

    /// @brief Whether climb is marked as favorite.
    bool    favorite = false;

    /// @brief User rating (0-5).
    int     rating   = 0;
};

/**
 * @brief Repository for climb record persistence.
 *
 * Provides CRUD operations and queries for climbs in the database.
 */
class ClimbRepository
{
public:
    /**
     * @brief Constructs repository with database connection.
     * @param db Database connection.
     */
    explicit ClimbRepository(QSqlDatabase& db);

    /**
     * @brief Inserts a new climb record into the database.
     * @param climb Climb record to insert.
     * @return ID of inserted record, or -1 on error.
     */
    int  insertClimb(const ClimbRecord& climb);

    /**
     * @brief Updates an existing climb record.
     * @param climb Climb record with updated values.
     * @return True if update succeeded.
     */
    bool updateClimb(const ClimbRecord& climb);

    /**
     * @brief Soft-deletes a climb (marks deleted=1, locked=1).
     * @param climbId Climb ID to delete.
     * @return True if successful.
     */
    bool softDeleteClimb(int climbId);

    /**
     * @brief Hard-deletes a climb row (permanent removal).
     * @param climbId Climb ID to delete.
     * @return True if successful.
     */
    bool hardDeleteClimb(int climbId);

    /**
     * @brief Deletes all climbs for an activity (hard delete).
     * @param activityId Activity ID.
     * @return True if successful.
     */
    bool deleteClimbsForActivity(int activityId);

    /**
     * @brief Retrieves a single climb by ID.
     * @param climbId Climb ID.
     * @return Climb record.
     */
    ClimbRecord        getClimb(int climbId);

    /**
     * @brief Lists all non-deleted climbs for an activity.
     * @param activityId Activity ID.
     * @return List of climb records.
     */
    QList<ClimbRecord> climbsForActivity(int activityId);

    /**
     * @brief Lists all deleted climbs for an activity.
     * @param activityId Activity ID.
     * @return List of deleted climb records.
     */
    QList<ClimbRecord> deletedClimbsForActivity(int activityId);

    /**
     * @brief Checks if non-deleted climbs exist for an activity.
     * @param activityId Activity ID.
     * @return True if any non-deleted climbs found.
     */
    bool hasClimbs(int activityId);

    /**
     * @brief Checks if analysis has been run for an activity.
     * @param activityId Activity ID.
     * @return True if any climb records exist (deleted or not).
     */
    bool hasClimbsEver(int activityId);

    /**
     * @brief Checks if any locked non-deleted climbs exist.
     * @param activityId Activity ID.
     * @return True if locked climbs found.
     */
    bool hasLockedClimbs(int activityId);

private:
    QSqlDatabase& m_db;
};