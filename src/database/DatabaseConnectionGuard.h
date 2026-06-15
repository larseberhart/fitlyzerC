// SPDX-License-Identifier: GPL-3

/**
 * @file DatabaseConnectionGuard.h
 * @brief Database access component for DatabaseConnectionGuard.
 *
 * Implements database schema handling, repository operations, or SQL utility behavior for persistent storage.
 *
 * Responsibilities:
 * - Provide database schema, query, or repository functionality
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QSqlDatabase>
#include <QString>

/**
 * @brief RAII guard for temporary SQLite database connections.
 *
 * Ensures connections are properly closed and removed from Qt's connection registry,
 * even if exceptions occur during processing.
 */
class DatabaseConnectionGuard
{
public:
    /**
     * @brief Constructs guard with connection name.
     * @param connectionName Unique name for this connection.
     */
    explicit DatabaseConnectionGuard(const QString& connectionName);
    ~DatabaseConnectionGuard();

    // Non-copyable
    DatabaseConnectionGuard(const DatabaseConnectionGuard&) = delete;
    DatabaseConnectionGuard& operator=(const DatabaseConnectionGuard&) = delete;

    /**
     * @brief Gets the database connection.
     * @return QSqlDatabase for this guard.
     */
    QSqlDatabase database() const;

private:
    QString m_connectionName;
};