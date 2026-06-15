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

static void applySqlitePragmas(QSqlDatabase& db)
{
    QSqlQuery q(db);
    q.exec("PRAGMA journal_mode=WAL");
    q.exec("PRAGMA synchronous=NORMAL");
    q.exec("PRAGMA temp_store=MEMORY");
    q.exec("PRAGMA foreign_keys=ON");
    q.exec("PRAGMA cache_size=-20000");
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

static bool ensureActivityTimeColumns(QSqlDatabase& db, QString* errorOut)
{
    if (!columnExists(db, "activities", "activity_start_time"))
    {
        QSqlQuery q(db);
        if (!q.exec("ALTER TABLE activities ADD COLUMN activity_start_time TEXT"))
        {
            if (errorOut)
                *errorOut = q.lastError().text();
            return false;
        }
    }

    if (!columnExists(db, "activities", "import_time"))
    {
        QSqlQuery q(db);
        if (!q.exec("ALTER TABLE activities ADD COLUMN import_time TEXT"))
        {
            if (errorOut)
                *errorOut = q.lastError().text();
            return false;
        }
    }

    const bool hasLegacyStart = columnExists(db, "activities", "start_time");
    const bool hasLegacyImported = columnExists(db, "activities", "imported_at");

    QString startExpr = QStringLiteral("activity_start_time = COALESCE(activity_start_time");
    if (hasLegacyStart)
        startExpr += QStringLiteral(", start_time");
    if (hasLegacyImported)
        startExpr += QStringLiteral(", imported_at");
    startExpr += QStringLiteral(", datetime('now'))");

    QString importExpr = QStringLiteral("import_time = COALESCE(import_time");
    if (hasLegacyImported)
        importExpr += QStringLiteral(", imported_at");
    importExpr += QStringLiteral(", datetime('now'))");

    {
        QSqlQuery q(db);
        if (!q.exec(QString("UPDATE activities SET %1, %2").arg(startExpr, importExpr)))
        {
            if (errorOut)
                *errorOut = q.lastError().text();
            return false;
        }
    }

    {
        QSqlQuery q(db);
        q.exec("CREATE INDEX IF NOT EXISTS idx_activities_activity_start_time ON activities(activity_start_time DESC)");
    }

    return true;
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

    if (!ensureActivityTimeColumns(m_db, errorOut))
        return false;

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

    if (fromVersion < 4)
    {
        if (!ensureActivityTimeColumns(m_db, errorOut))
            return false;
    }

    if (fromVersion < 5)
    {
        QSqlQuery q(m_db);
        q.exec(
            "CREATE INDEX IF NOT EXISTS idx_activities_athlete_start_time"
            " ON activities(athlete_id, activity_start_time DESC)");
    }

    if (fromVersion < 6)
    {
        // Add climbs table
        {
            QSqlQuery q(m_db);
            q.exec(
                "CREATE TABLE IF NOT EXISTS climbs ("
                "  id               INTEGER PRIMARY KEY AUTOINCREMENT,"
                "  activity_id      INTEGER NOT NULL REFERENCES activities(id) ON DELETE CASCADE,"
                "  start_seconds    REAL    NOT NULL,"
                "  end_seconds      REAL    NOT NULL,"
                "  name             TEXT,"
                "  elevation_gain_m REAL,"
                "  length_km        REAL,"
                "  average_gradient REAL,"
                "  source           TEXT    NOT NULL DEFAULT 'auto',"
                "  locked           INTEGER NOT NULL DEFAULT 0,"
                "  created_at       TEXT    DEFAULT CURRENT_TIMESTAMP,"
                "  updated_at       TEXT    DEFAULT CURRENT_TIMESTAMP"
                ")");
        }
        {
            QSqlQuery q(m_db);
            q.exec("CREATE INDEX IF NOT EXISTS idx_climbs_activity_id ON climbs(activity_id)");
        }

        // Add source, locked, updated_at to intervals
        struct ColDef { const char* name; const char* definition; };
        static const ColDef kIntervalCols[] = {
            { "source",     "TEXT NOT NULL DEFAULT 'auto'" },
            { "locked",     "INTEGER NOT NULL DEFAULT 0"   },
            { "updated_at", "TEXT"                         },
        };
        for (const ColDef& col : kIntervalCols)
        {
            if (columnExists(m_db, "intervals", col.name))
                continue;
            QSqlQuery q(m_db);
            q.exec(QString("ALTER TABLE intervals ADD COLUMN %1 %2")
                   .arg(col.name).arg(col.definition));
        }

        // Indexes on activities for fast overlap / hash lookup
        {
            QSqlQuery q(m_db);
            q.exec(
                "CREATE UNIQUE INDEX IF NOT EXISTS idx_activity_filehash"
                " ON activities(fit_hash) WHERE fit_hash IS NOT NULL");
        }
        {
            QSqlQuery q(m_db);
            q.exec("CREATE INDEX IF NOT EXISTS idx_activity_start_time ON activities(activity_start_time)");
        }
        {
            QSqlQuery q(m_db);
            q.exec("CREATE INDEX IF NOT EXISTS idx_activity_end_time ON activities(end_time)");
        }
    }

    if (fromVersion < 7)
    {
        // activities: analysis_version, fingerprint, analysis_flags, import_source
        struct ColDef7a { const char* name; const char* def; };
        static const ColDef7a kActivityCols[] = {
            { "analysis_version", "INTEGER NOT NULL DEFAULT 1"  },
            { "fingerprint",      "TEXT"                        },
            { "analysis_flags",   "INTEGER NOT NULL DEFAULT 0"  },
            { "import_source",    "TEXT"                        },
        };
        for (const ColDef7a& col : kActivityCols)
        {
            if (columnExists(m_db, "activities", col.name)) continue;
            QSqlQuery q(m_db);
            q.exec(QString("ALTER TABLE activities ADD COLUMN %1 %2").arg(col.name).arg(col.def));
        }

        // intervals: algorithm_version, deleted, uuid
        struct ColDef7b { const char* name; const char* def; };
        static const ColDef7b kIntervalCols[] = {
            { "algorithm_version", "INTEGER NOT NULL DEFAULT 1"  },
            { "deleted",           "INTEGER NOT NULL DEFAULT 0"  },
            { "uuid",              "TEXT"                        },
        };
        for (const ColDef7b& col : kIntervalCols)
        {
            if (columnExists(m_db, "intervals", col.name)) continue;
            QSqlQuery q(m_db);
            q.exec(QString("ALTER TABLE intervals ADD COLUMN %1 %2").arg(col.name).arg(col.def));
        }

        // climbs: algorithm_version, deleted, uuid, original bounds, statistics, notes, favorite, rating
        struct ColDef7c { const char* name; const char* def; };
        static const ColDef7c kClimbCols[] = {
            { "algorithm_version",      "INTEGER NOT NULL DEFAULT 1"  },
            { "deleted",                "INTEGER NOT NULL DEFAULT 0"  },
            { "uuid",                   "TEXT"                        },
            { "original_start_seconds", "REAL"                        },
            { "original_end_seconds",   "REAL"                        },
            { "avg_power",              "REAL"                        },
            { "np",                     "REAL"                        },
            { "avg_hr",                 "REAL"                        },
            { "avg_cadence",            "REAL"                        },
            { "vam",                    "REAL"                        },
            { "notes",                  "TEXT"                        },
            { "favorite",               "INTEGER NOT NULL DEFAULT 0"  },
            { "rating",                 "INTEGER NOT NULL DEFAULT 0"  },
        };
        for (const ColDef7c& col : kClimbCols)
        {
            if (columnExists(m_db, "climbs", col.name)) continue;
            QSqlQuery q(m_db);
            q.exec(QString("ALTER TABLE climbs ADD COLUMN %1 %2").arg(col.name).arg(col.def));
        }

        // new tables
        {
            QSqlQuery q(m_db);
            q.exec(
                "CREATE TABLE IF NOT EXISTS activity_analysis_settings ("
                "  activity_id             INTEGER PRIMARY KEY,"
                "  climb_min_gain          REAL,"
                "  climb_min_length        REAL,"
                "  climb_min_gradient      REAL,"
                "  interval_work_threshold REAL,"
                "  interval_rest_threshold REAL,"
                "  FOREIGN KEY(activity_id) REFERENCES activities(id) ON DELETE CASCADE"
                ")");
        }
        {
            QSqlQuery q(m_db);
            q.exec(
                "CREATE TABLE IF NOT EXISTS activity_analysis_cache ("
                "  activity_id INTEGER NOT NULL,"
                "  cache_key   TEXT    NOT NULL,"
                "  cache_value BLOB,"
                "  PRIMARY KEY (activity_id, cache_key)"
                ")");
        }
    }

    if (fromVersion < 8)
    {
        // Version 8: Add performance-critical indexes
        {
            QSqlQuery q(m_db);
            q.exec("CREATE INDEX IF NOT EXISTS idx_activity_samples_time"
                   " ON activity_samples(activity_id, elapsed_seconds)");
        }
        {
            QSqlQuery q(m_db);
            q.exec("CREATE INDEX IF NOT EXISTS idx_imports_date"
                   " ON imports(imported_at)");
        }
        {
            QSqlQuery q(m_db);
            q.exec("CREATE INDEX IF NOT EXISTS idx_workouts_athlete_date"
                   " ON planned_workouts(athlete_id, workout_date)");
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

    applySqlitePragmas(m_db);

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
