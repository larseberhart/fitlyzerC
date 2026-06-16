// SPDX-License-Identifier: GPL-3


#pragma once

#include <QList>
#include <QString>

class QSqlDatabase;

/**
 * @brief Log entry for an activity import.
 */
struct ImportRecord
{
    /// @brief Unique import log entry ID.
    int     id           = -1;

    /// @brief Name of imported file.
    QString fileName;

    /// @brief ISO8601 import timestamp.
    QString importedAt;

    /// @brief Import status (success, duplicate, error).
    QString status;

    /// @brief ID of imported activity (or -1 if failed).
    int     activityId   = -1;

    /// @brief Error message if import failed.
    QString errorMessage;
};

/**
 * @brief Repository for import log persistence.
 *
 * Tracks all import attempts including successes, duplicates, and errors.
 */
class ImportRepository
{
public:
    /**
     * @brief Constructs repository with database connection.
     * @param db Database connection.
     */
    explicit ImportRepository(QSqlDatabase& db);

    /**
     * @brief Logs an import attempt.
     * @param r Import record to log.
     * @return ID of log entry, or -1 on error.
     */
    int              logImport(const ImportRecord& r);

    /**
     * @brief Retrieves recent import log entries.
     * @param limit Maximum number of entries to return (default 50).
     * @return List of import records.
     */
    QList<ImportRecord> getRecentImports(int limit = 50);

private:
    QSqlDatabase& m_db;
};