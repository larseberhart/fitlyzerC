// SPDX-License-Identifier: GPL-3


#pragma once

#include <string_view>

#include <QSqlQuery>
#include <QString>

/// Common SQL patterns and query builders to reduce duplication across repositories.
/**
 * @brief Helper utilities for SQL query generation and binding.
 */
namespace SqlQueryHelper {

/// @brief SQL predicate for non-deleted rows.
inline constexpr std::string_view kWhereNotDeleted = "COALESCE(deleted,0)=0";

/// @brief SQL predicate for deleted rows.
inline constexpr std::string_view kWhereDeleted = "COALESCE(deleted,0)=1";

/// @brief SQL predicate for locked rows.
inline constexpr std::string_view kWhereLocked = "locked=1";

/**
 * @brief Binds a boolean value to a query placeholder (0 or 1).
 * @param query SQL query to bind to.
 * @param placeholder Placeholder name (e.g., ":locked").
 * @param value Boolean value to bind.
 */
inline void bindBoolValue(QSqlQuery& query, const QString& placeholder, bool value)
{
    query.bindValue(placeholder, value ? 1 : 0);
}

/**
 * @brief Generates EXISTS query for non-deleted records.
 * @param tableName Table to query.
 * @return Query string (requires :id parameter).
 */
inline QString selectExistsNonDeleted(const QString& tableName)
{
    return QString("SELECT 1 FROM %1 WHERE activity_id=:id AND %2 LIMIT 1")
        .arg(tableName, QString::fromUtf8(kWhereNotDeleted.data(), kWhereNotDeleted.size()));
}

/**
 * @brief Generates EXISTS query for locked records.
 * @param tableName Table to query.
 * @return Query string (requires :id parameter).
 */
inline QString selectExistsLocked(const QString& tableName)
{
    return QString("SELECT 1 FROM %1 WHERE activity_id=:id AND %2 AND %3 LIMIT 1")
        .arg(tableName, QString::fromUtf8(kWhereLocked.data(), kWhereLocked.size()),
             QString::fromUtf8(kWhereNotDeleted.data(), kWhereNotDeleted.size()));
}

/**
 * @brief Generates EXISTS query regardless of deletion state.
 * @param tableName Table to query.
 * @return Query string (requires :id parameter).
 */
inline QString selectExistsEver(const QString& tableName)
{
    return QString("SELECT 1 FROM %1 WHERE activity_id=:id LIMIT 1").arg(tableName);
}

}  // namespace SqlQueryHelper