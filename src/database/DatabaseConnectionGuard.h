#pragma once

#include <QSqlDatabase>
#include <QString>

/**
 * RAII guard for temporary SQLite database connections.
 * Ensures connections are properly closed and removed from Qt's connection registry,
 * even if exceptions occur during processing.
 *
 * Usage:
 *   DatabaseConnectionGuard guard("temp_conn_name");
 *   QSqlDatabase db = guard.database();
 *   db.setDatabaseName("path/to/db");
 *   db.open();
 *   // ... use db ...
 *   // Automatic cleanup on destruction
 */
class DatabaseConnectionGuard
{
public:
    explicit DatabaseConnectionGuard(const QString& connectionName);
    ~DatabaseConnectionGuard();

    // Non-copyable
    DatabaseConnectionGuard(const DatabaseConnectionGuard&) = delete;
    DatabaseConnectionGuard& operator=(const DatabaseConnectionGuard&) = delete;

    // Get the database connection
    QSqlDatabase database() const;

private:
    QString m_connectionName;
};
