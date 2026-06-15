// SPDX-License-Identifier: GPL-3

/**
 * @file ActivityAnalysisSettingsRepository.h
 * @brief Database access component for ActivityAnalysisSettingsRepository.
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
 * @brief Analysis settings per activity.
 */
struct ActivityAnalysisSettings
{
    /// @brief Associated activity ID.
    int    activityId             = -1;

    /// @brief Minimum climb elevation gain in meters.
    double climbMinGain           = 30.0;

    /// @brief Minimum climb length in kilometers.
    double climbMinLength         = 0.5;

    /// @brief Minimum climb gradient percentage.
    double climbMinGradient       = 3.0;

    /// @brief Work interval power threshold (fraction of FTP).
    double intervalWorkThreshold  = 0.0;

    /// @brief Recovery interval power threshold (fraction of FTP).
    double intervalRestThreshold  = 0.0;

    /**
     * @brief Checks if settings are valid.
     */
    bool isValid() const { return activityId > 0; }
};

/**
 * @brief Repository for per-activity analysis settings.
 *
 * Stores custom climb detection and interval detection parameters per activity.
 */
class ActivityAnalysisSettingsRepository
{
public:
    /**
     * @brief Constructs repository with database connection.
     * @param db Database connection.
     */
    explicit ActivityAnalysisSettingsRepository(QSqlDatabase& db);

    /**
     * @brief Saves or updates activity settings.
     * @param settings Settings to save.
     * @return True on success.
     */
    bool insertOrUpdate(const ActivityAnalysisSettings& settings);

    /**
     * @brief Retrieves settings for an activity.
     * @param activityId Activity ID.
     * @return Settings, or invalid if not found.
     */
    ActivityAnalysisSettings settingsForActivity(int activityId);

private:
    QSqlDatabase& m_db;
};