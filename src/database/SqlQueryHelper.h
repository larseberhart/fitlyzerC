#pragma once

#include <string_view>

#include <QSqlQuery>
#include <QString>

/// Common SQL patterns and query builders to reduce duplication across repositories.
namespace SqlQueryHelper {

// Common WHERE clauses for soft-delete logic
inline constexpr std::string_view kWhereNotDeleted = "COALESCE(deleted,0)=0";
inline constexpr std::string_view kWhereDeleted = "COALESCE(deleted,0)=1";
inline constexpr std::string_view kWhereLocked = "locked=1";

// Commonly used bind patterns
// Use as: bindBoolValue(q, ":locked", record.locked)
inline void bindBoolValue(QSqlQuery& query, const QString& placeholder, bool value)
{
    query.bindValue(placeholder, value ? 1 : 0);
}

// String builder helpers for common EXISTS/COUNT patterns
inline QString selectExistsNonDeleted(const QString& tableName)
{
    return QString("SELECT 1 FROM %1 WHERE activity_id=:id AND %2 LIMIT 1")
        .arg(tableName, QString::fromUtf8(kWhereNotDeleted.data(), kWhereNotDeleted.size()));
}

inline QString selectExistsLocked(const QString& tableName)
{
    return QString("SELECT 1 FROM %1 WHERE activity_id=:id AND %2 AND %3 LIMIT 1")
        .arg(tableName, QString::fromUtf8(kWhereLocked.data(), kWhereLocked.size()),
             QString::fromUtf8(kWhereNotDeleted.data(), kWhereNotDeleted.size()));
}

inline QString selectExistsEver(const QString& tableName)
{
    return QString("SELECT 1 FROM %1 WHERE activity_id=:id LIMIT 1").arg(tableName);
}

}  // namespace SqlQueryHelper
