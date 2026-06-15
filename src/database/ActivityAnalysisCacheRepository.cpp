// SPDX-License-Identifier: GPL-3

/**
 * @file ActivityAnalysisCacheRepository.cpp
 * @brief Database access component for ActivityAnalysisCacheRepository.
 *
 * Implements database schema handling, repository operations, or SQL utility behavior for persistent storage.
 *
 * Responsibilities:
 * - Provide database schema, query, or repository functionality
 *
 * @author Lars EBERHART
 */

#include "ActivityAnalysisCacheRepository.h"

#include <QSqlDatabase>
#include <QSqlQuery>

ActivityAnalysisCacheRepository::ActivityAnalysisCacheRepository(QSqlDatabase& db)
    : m_db(db)
{}

bool ActivityAnalysisCacheRepository::setCache(int activityId, const QString& key, const QByteArray& value)
{
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT OR REPLACE INTO activity_analysis_cache(activity_id, cache_key, cache_value)"
        " VALUES(:id, :k, :v)");
    q.bindValue(":id", activityId);
    q.bindValue(":k",  key);
    q.bindValue(":v",  value);
    return q.exec();
}

QByteArray ActivityAnalysisCacheRepository::getCache(int activityId, const QString& key)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT cache_value FROM activity_analysis_cache WHERE activity_id=:id AND cache_key=:k");
    q.bindValue(":id", activityId);
    q.bindValue(":k",  key);
    if (q.exec() && q.next())
        return q.value(0).toByteArray();
    return {};
}

bool ActivityAnalysisCacheRepository::hasCache(int activityId, const QString& key)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT 1 FROM activity_analysis_cache WHERE activity_id=:id AND cache_key=:k LIMIT 1");
    q.bindValue(":id", activityId);
    q.bindValue(":k",  key);
    return q.exec() && q.next();
}

bool ActivityAnalysisCacheRepository::invalidateCache(int activityId)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM activity_analysis_cache WHERE activity_id=:id");
    q.bindValue(":id", activityId);
    return q.exec();
}

bool ActivityAnalysisCacheRepository::invalidateKey(int activityId, const QString& key)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM activity_analysis_cache WHERE activity_id=:id AND cache_key=:k");
    q.bindValue(":id", activityId);
    q.bindValue(":k",  key);
    return q.exec();
}