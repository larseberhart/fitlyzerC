#include "DatabaseManager.h"
#include "DatabaseSchema.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

DatabaseManager::DatabaseManager() = default;

DatabaseManager::~DatabaseManager()
{
    close();
}

// ── Private helpers ───────────────────────────────────────────────────────────

static void enableWAL(QSqlDatabase& db)
{
    QSqlQuery q(db);
    q.exec("PRAGMA journal_mode=WAL");
    q.exec("PRAGMA foreign_keys=ON");
    q.exec("PRAGMA synchronous=NORMAL");
}

static bool columnExists(QSqlDatabase& db, const QString& tableName, const QString& columnName)
{
    QSqlQuery q(db);
    q.exec(QString("PRAGMA table_info(%1)").arg(tableName));
    while (q.next())
    {
        if (q.value(1).toString().compare(columnName, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

bool DatabaseManager::applySchema(QString* errorOut)
{
    QSqlQuery q(m_db);

    // Run every DDL statement from the schema
    for (const auto& stmt : DatabaseSchema::kStatements)
    {
        const QString sql = QString::fromUtf8(stmt.data(), static_cast<qsizetype>(stmt.size()));
        if (!q.exec(sql))
        {
            if (errorOut)
                *errorOut = q.lastError().text();
            return false;
        }
    }

    // Write schema version
    q.prepare("INSERT OR REPLACE INTO schema_info(version, created_at) VALUES(:v, :ts)");
    q.bindValue(":v",  DatabaseSchema::kCurrentVersion);
    q.bindValue(":ts", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!q.exec())
    {
        if (errorOut)
            *errorOut = q.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::upgradeSchema(int fromVersion, QString* errorOut)
{
    if (fromVersion < 2 && !columnExists(m_db, "athletes", "ftp"))
    {
        QSqlQuery q(m_db);
        if (!q.exec("ALTER TABLE athletes ADD COLUMN ftp INTEGER NOT NULL DEFAULT 250"))
        {
            if (errorOut)
                *errorOut = q.lastError().text();
            return false;
        }
    }

    if (fromVersion < 3)
    {
        struct ActivityColumnDef
        {
            const char* name;
            const char* definition;
        };

        static const ActivityColumnDef kActivityColumns[] = {
            {"temperature", "REAL"},
            {"weather", "TEXT"},
            {"wind", "TEXT"},
            {"rpe", "INTEGER"},
            {"fatigue", "INTEGER"},
            {"sleep", "REAL"},
            {"weight", "REAL"},
            {"bike", "TEXT"},
            {"equipment", "TEXT"},
        };

        for (const ActivityColumnDef& col : kActivityColumns)
        {
            if (columnExists(m_db, "activities", col.name))
                continue;

            QSqlQuery q(m_db);
            const QString sql = QString("ALTER TABLE activities ADD COLUMN %1 %2")
                .arg(col.name)
                .arg(col.definition);
            if (!q.exec(sql))
            {
                if (errorOut)
                    *errorOut = q.lastError().text();
                return false;
            }
        }
    }

    return applySchema(errorOut);
}

// ── Public API ────────────────────────────────────────────────────────────────

bool DatabaseManager::open(const QString& path, QString* errorOut)
{
    close();

    m_connectionName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(path);

    if (!m_db.open())
    {
        if (errorOut)
            *errorOut = m_db.lastError().text();
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connectionName.clear();
        return false;
    }

    enableWAL(m_db);

    // Check schema version
    QSqlQuery q(m_db);
    q.exec("SELECT version FROM schema_info ORDER BY rowid DESC LIMIT 1");
    if (q.next())
    {
        const int version = q.value(0).toInt();
        if (version < DatabaseSchema::kCurrentVersion)
        {
            if (!upgradeSchema(version, errorOut))
                return false;
        }
    }
    else
    {
        // schema_info missing: fresh file or legacy — apply schema
        if (!applySchema(errorOut))
            return false;
    }

    m_path = path;
    return true;
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
