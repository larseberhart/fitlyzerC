// SPDX-License-Identifier: GPL-3


#include "DatabaseManager.h"
#include "DatabaseSchema.h"
#include "database/connection/DatabaseConnectionFactory.h"
#include "database/migrations/DatabaseMigrationManager.h"

#include <QSqlError>
#include <QSqlQuery>

DatabaseManager::DatabaseManager() = default;

DatabaseManager::~DatabaseManager()
{
    close();
}

// ── Private helpers ───────────────────────────────────────────────────────────

bool DatabaseManager::applySchema(QString* errorOut)
{
    return DatabaseMigrationManager::applySchema(m_db, errorOut);
}

bool DatabaseManager::upgradeSchema(int fromVersion, QString* errorOut)
{
    return DatabaseMigrationManager::upgradeSchema(m_db, fromVersion, errorOut);
}

// ── Public API ────────────────────────────────────────────────────────────────

bool DatabaseManager::open(const QString& path, QString* errorOut)
{
    close();

    m_connectionName = DatabaseConnectionFactory::createConnectionName();
    if (!DatabaseConnectionFactory::openSqlite(m_db, path, m_connectionName, errorOut))
    {
        m_connectionName.clear();
        return false;
    }

    DatabaseConnectionFactory::applySqlitePragmas(m_db);

    // Check schema version
    QSqlQuery q(m_db);
    q.exec("SELECT version FROM schema_info ORDER BY rowid DESC LIMIT 1");
    if (q.next())
    {
        const int version = q.value(0).toInt();
        if (version < DatabaseSchema::kCurrentVersion)
        {
            if (!upgradeSchema(version, errorOut))
            {
                cleanupFailedOpen();
                return false;
            }
        }
    }
    else
    {
        // schema_info missing: fresh file or legacy — apply schema
        if (!applySchema(errorOut))
        {
            cleanupFailedOpen();
            return false;
        }
    }

    m_path = path;
    return true;
}

void DatabaseManager::cleanupFailedOpen()
{
    if (m_db.isOpen())
        m_db.close();

    if (!m_connectionName.isEmpty())
    {
        QString conn = m_connectionName;
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(conn);
        m_connectionName.clear();
    }
}

bool DatabaseManager::create(const QString& path, QString* errorOut)
{
    // QSqlite creates the file if it doesn't exist — same as open
    return open(path, errorOut);
}

void DatabaseManager::close()
{
    if (m_db.isOpen())
        m_db.close();

    if (!m_connectionName.isEmpty())
    {
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connectionName.clear();
    }

    m_path.clear();
    m_db = QSqlDatabase(); // reset to default-constructed
}

bool DatabaseManager::isOpen() const
{
    return m_db.isValid() && m_db.isOpen();
}

QSqlDatabase DatabaseManager::database() const
{
    return m_db;
}