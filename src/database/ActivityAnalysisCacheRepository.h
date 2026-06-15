// SPDX-License-Identifier: GPL-3

/**
 * @file ActivityAnalysisCacheRepository.h
 * @brief Database access component for ActivityAnalysisCacheRepository.
 *
 * Implements database schema handling, repository operations, or SQL utility behavior for persistent storage.
 *
 * Responsibilities:
 * - Provide database schema, query, or repository functionality
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QByteArray>
#include <QString>

class QSqlDatabase;

/**
 * @brief Caches expensive analysis results per activity.
 *
 * Stores pre-computed power curves, histograms, best efforts, and other
 * analysis data to improve responsiveness on repeated access.
 */
class ActivityAnalysisCacheRepository
{
public:
    /**
     * @brief Constructs repository with database connection.
     * @param db Database connection.
     */
    explicit ActivityAnalysisCacheRepository(QSqlDatabase& db);

    /**
     * @brief Stores analysis result cache.
     * @param activityId Activity ID.
     * @param key Cache key name.
     * @param value Serialized cache data.
     * @return True on success.
     */
    bool       setCache(int activityId, const QString& key, const QByteArray& value);

    /**
     * @brief Retrieves cached analysis result.
     * @param activityId Activity ID.
     * @param key Cache key name.
     * @return Cached data, or empty if not found.
     */
    QByteArray getCache(int activityId, const QString& key);

    /**
     * @brief Checks if cache entry exists.
     * @param activityId Activity ID.
     * @param key Cache key name.
     * @return True if entry exists.
     */
    bool       hasCache(int activityId, const QString& key);

    /**
     * @brief Invalidates all cache for an activity.
     * @param activityId Activity ID.
     * @return True on success.
     */
    bool       invalidateCache(int activityId);

    /**
     * @brief Invalidates specific cache key for an activity.
     * @param activityId Activity ID.
     * @param key Cache key name.
     * @return True on success.
     */
    bool       invalidateKey(int activityId, const QString& key);

private:
    QSqlDatabase& m_db;
};