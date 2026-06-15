// SPDX-License-Identifier: GPL-3

/**
 * @file DatabaseManager.h
 * @brief Database access component for DatabaseManager.
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

// DatabaseManager owns a single named QSqlDatabase connection.
// Usage:
//   DatabaseManager db;
//   if (!db.open("/path/to/file.fitlyzerdb", &errorMsg)) { ... }
//   db.database(); // use for queries
/**
 * @brief Manages a single database connection and schema for FitlyzerC.
 *
 * Handles opening, creating, and closing SQLite database connections,
 * applying schema initialization and upgrades as needed.
 */
class DatabaseManager
{
public:
    DatabaseManager();
    ~DatabaseManager();

    /**
     * @brief Opens an existing database file.
     * @param path Path to database file.
     * @param errorOut Optional error message output.
     * @return True if successfully opened, false otherwise.
     */
    bool open(const QString& path, QString* errorOut = nullptr);

    /**
     * @brief Creates a new database file at the specified path.
     * @param path Path for new database file.
     * @param errorOut Optional error message output.
     * @return True if successfully created, false otherwise.
     */
    bool create(const QString& path, QString* errorOut = nullptr);

    /**
     * @brief Closes the database connection.
     */
    void close();

    /**
     * @brief Checks if database connection is open.
     * @return True if connected to a database.
     */
    bool isOpen() const;

    /**
     * @brief Gets the underlying SQL database connection.
     * @return QSqlDatabase handle (valid only when isOpen() returns true).
     */
    QSqlDatabase database() const;

    /**
     * @brief Gets the database file path.
     * @return Path to the database file.
     */
    QString path() const { return m_path; }

private:
    bool applySchema(QString* errorOut);
    bool upgradeSchema(int fromVersion, QString* errorOut);
    void cleanupFailedOpen();

    QString      m_path;
    QString      m_connectionName;
    QSqlDatabase m_db;
};