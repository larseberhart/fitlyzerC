#pragma once

#include <QSqlDatabase>
#include <QString>

// DatabaseManager owns a single named QSqlDatabase connection.
// Usage:
//   DatabaseManager db;
//   if (!db.open("/path/to/file.fitlyzerdb", &errorMsg)) { ... }
//   db.database(); // use for queries
class DatabaseManager
{
public:
    DatabaseManager();
    ~DatabaseManager();

    // Open an existing database file. Returns false and sets errorOut on failure.
    bool open(const QString& path, QString* errorOut = nullptr);

    // Create a new database file at path (must not already exist).
    // Returns false and sets errorOut on failure.
    bool create(const QString& path, QString* errorOut = nullptr);

    void close();

    bool isOpen() const;

    // The underlying connection (valid only when isOpen()).
    QSqlDatabase database() const;

    QString path() const { return m_path; }

private:
    bool applySchema(QString* errorOut);
    bool upgradeSchema(int fromVersion, QString* errorOut);
    void cleanupFailedOpen();

    QString      m_path;
    QString      m_connectionName;
    QSqlDatabase m_db;
};
