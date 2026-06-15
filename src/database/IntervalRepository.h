// SPDX-License-Identifier: GPL-3

/**
 * @file IntervalRepository.h
 * @brief Database access component for IntervalRepository.
 *
 * Implements database schema handling, repository operations, or SQL utility behavior for persistent storage.
 *
 * Responsibilities:
 * - Provide database schema, query, or repository functionality
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QList>
#include <QString>

class QSqlDatabase;

/**
 * @brief Database representation of an interval record.
 */
struct IntervalRecord
{
    /// @brief Unique interval record ID.
    int id = -1;

    /// @brief Associated activity ID.
    int activityId = -1;

    /// @brief Starting sample index.
    int startSample = 0;

    /// @brief Ending sample index.
    int endSample = 0;

    /// @brief Interval name.
    QString name;

    /// @brief Interval type (Work, Recovery, etc.).
    QString type;

    /// @brief Average power in watts.
    double avgPower = 0.0;

    /// @brief Normalized power in watts.
    double np = 0.0;

    /// @brief Average heart rate in bpm.
    double avgHr = 0.0;

    /// @brief Average cadence in rpm.
    double avgCadence = 0.0;

    /// @brief User notes about interval.
    QString notes;

    /// @brief Source of interval (auto, manual, edited).
    QString source           = QStringLiteral("auto");

    /// @brief Whether interval is locked from editing.
    bool    locked           = false;

    /// @brief Soft-delete flag.
    bool    deleted          = false;

    /// @brief Algorithm version used for detection.
    int     algorithmVersion = 1;

    /// @brief Stable UUID, generated once on insert.
    QString uuid;
};

/**
 * @brief Repository for interval record persistence.
 *
 * Provides CRUD operations and queries for work/recovery intervals in the database.
 */
class IntervalRepository
{
public:
    /**
     * @brief Constructs interval repository.
     * @param db SQL database reference.
     */
    explicit IntervalRepository(QSqlDatabase& db);

    /**
     * @brief Inserts a new interval record.
     * @param interval Interval to insert.
     * @return New interval ID, or -1 on error.
     */
    int  insertInterval(const IntervalRecord& interval);

    /**
     * @brief Updates an existing interval record.
     * @param interval Interval with updated data.
     * @return True if update succeeded.
     */
    bool updateInterval(const IntervalRecord& interval);

    /// Soft-delete: marks deleted=1.
    bool softDeleteInterval(int intervalId);
    /// Hard-delete (for full reset before re-detect).
    bool deleteInterval(int intervalId);

    /**
     * @brief Deletes all intervals for an activity.
     * @param activityId Activity ID.
     * @return True if deletion succeeded.
     */
    bool deleteIntervalsForActivity(int activityId);

    /// Returns non-deleted intervals only.
    QList<IntervalRecord> listIntervalsForActivity(int activityId);

    /**
     * @brief Checks if activity has locked intervals.
     * @param activityId Activity ID.
     * @return True if locked intervals exist.
     */
    bool hasLockedIntervals(int activityId);

private:
    QSqlDatabase& m_db;
};