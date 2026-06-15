// SPDX-License-Identifier: GPL-3

/**
 * @file ActivityAnalysisSettingsRepository.cpp
 * @brief Database access component for ActivityAnalysisSettingsRepository.
 *
 * Implements database schema handling, repository operations, or SQL utility behavior for persistent storage.
 *
 * Responsibilities:
 * - Provide database schema, query, or repository functionality
 *
 * @author Lars EBERHART
 */

#include "ActivityAnalysisSettingsRepository.h"

#include <QSqlDatabase>
#include <QSqlQuery>

ActivityAnalysisSettingsRepository::ActivityAnalysisSettingsRepository(QSqlDatabase& db)
    : m_db(db)
{}

bool ActivityAnalysisSettingsRepository::insertOrUpdate(const ActivityAnalysisSettings& settings)
{
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT OR REPLACE INTO activity_analysis_settings("
        "  activity_id, climb_min_gain, climb_min_length, climb_min_gradient,"
        "  interval_work_threshold, interval_rest_threshold"
        ") VALUES("
        "  :activity_id, :climb_min_gain, :climb_min_length, :climb_min_gradient,"
        "  :interval_work_threshold, :interval_rest_threshold"
        ")");

    q.bindValue(":activity_id",             settings.activityId);
    q.bindValue(":climb_min_gain",           settings.climbMinGain);
    q.bindValue(":climb_min_length",         settings.climbMinLength);
    q.bindValue(":climb_min_gradient",       settings.climbMinGradient);
    q.bindValue(":interval_work_threshold",  settings.intervalWorkThreshold);
    q.bindValue(":interval_rest_threshold",  settings.intervalRestThreshold);
    return q.exec();
}

ActivityAnalysisSettings ActivityAnalysisSettingsRepository::settingsForActivity(int activityId)
{
    ActivityAnalysisSettings s;
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT activity_id, climb_min_gain, climb_min_length, climb_min_gradient,"
        "  interval_work_threshold, interval_rest_threshold"
        " FROM activity_analysis_settings WHERE activity_id=:id");
    q.bindValue(":id", activityId);
    if (!q.exec() || !q.next())
        return s;

    s.activityId            = q.value(0).toInt();
    s.climbMinGain          = q.value(1).toDouble();
    s.climbMinLength        = q.value(2).toDouble();
    s.climbMinGradient      = q.value(3).toDouble();
    s.intervalWorkThreshold = q.value(4).toDouble();
    s.intervalRestThreshold = q.value(5).toDouble();
    return s;
}