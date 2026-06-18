// SPDX-License-Identifier: GPL-3

#include "DatabaseMigrationManager.h"

#include "database/DatabaseSchema.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>

namespace
{
bool columnExists(QSqlDatabase& db, const QString& tableName, const QString& columnName)
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

bool ensureActivityTimeColumns(QSqlDatabase& db, QString* errorOut)
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
}

namespace DatabaseMigrationManager
{
bool applySchema(QSqlDatabase& db, QString* errorOut)
{
    QSqlQuery q(db);

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

    if (!ensureActivityTimeColumns(db, errorOut))
        return false;

    q.prepare("INSERT OR REPLACE INTO schema_info(version, created_at) VALUES(:v, :ts)");
    q.bindValue(":v", DatabaseSchema::kCurrentVersion);
    q.bindValue(":ts", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!q.exec())
    {
        if (errorOut)
            *errorOut = q.lastError().text();
        return false;
    }

    return true;
}

bool upgradeSchema(QSqlDatabase& db, int fromVersion, QString* errorOut)
{
    if (fromVersion < 2 && !columnExists(db, "athletes", "ftp"))
    {
        QSqlQuery q(db);
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
            if (columnExists(db, "activities", col.name))
                continue;

            QSqlQuery q(db);
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
        if (!ensureActivityTimeColumns(db, errorOut))
            return false;
    }

    if (fromVersion < 5)
    {
        QSqlQuery q(db);
        q.exec(
            "CREATE INDEX IF NOT EXISTS idx_activities_athlete_start_time"
            " ON activities(athlete_id, activity_start_time DESC)");
    }

    if (fromVersion < 6)
    {
        {
            QSqlQuery q(db);
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
            QSqlQuery q(db);
            q.exec("CREATE INDEX IF NOT EXISTS idx_climbs_activity_id ON climbs(activity_id)");
        }

        struct ColDef { const char* name; const char* definition; };
        static const ColDef kIntervalCols[] = {
            { "source",     "TEXT NOT NULL DEFAULT 'auto'" },
            { "locked",     "INTEGER NOT NULL DEFAULT 0"   },
            { "updated_at", "TEXT"                         },
        };
        for (const ColDef& col : kIntervalCols)
        {
            if (columnExists(db, "intervals", col.name))
                continue;
            QSqlQuery q(db);
            q.exec(QString("ALTER TABLE intervals ADD COLUMN %1 %2")
                   .arg(col.name).arg(col.definition));
        }

        {
            QSqlQuery q(db);
            q.exec(
                "CREATE UNIQUE INDEX IF NOT EXISTS idx_activity_filehash"
                " ON activities(fit_hash) WHERE fit_hash IS NOT NULL");
        }
        {
            QSqlQuery q(db);
            q.exec("CREATE INDEX IF NOT EXISTS idx_activity_start_time ON activities(activity_start_time)");
        }
        {
            QSqlQuery q(db);
            q.exec("CREATE INDEX IF NOT EXISTS idx_activity_end_time ON activities(end_time)");
        }
    }

    if (fromVersion < 7)
    {
        struct ColDef7a { const char* name; const char* def; };
        static const ColDef7a kActivityCols[] = {
            { "analysis_version", "INTEGER NOT NULL DEFAULT 1"  },
            { "fingerprint",      "TEXT"                        },
            { "analysis_flags",   "INTEGER NOT NULL DEFAULT 0"  },
            { "import_source",    "TEXT"                        },
        };
        for (const ColDef7a& col : kActivityCols)
        {
            if (columnExists(db, "activities", col.name)) continue;
            QSqlQuery q(db);
            q.exec(QString("ALTER TABLE activities ADD COLUMN %1 %2").arg(col.name).arg(col.def));
        }

        struct ColDef7b { const char* name; const char* def; };
        static const ColDef7b kIntervalCols[] = {
            { "algorithm_version", "INTEGER NOT NULL DEFAULT 1"  },
            { "deleted",           "INTEGER NOT NULL DEFAULT 0"  },
            { "uuid",              "TEXT"                        },
        };
        for (const ColDef7b& col : kIntervalCols)
        {
            if (columnExists(db, "intervals", col.name)) continue;
            QSqlQuery q(db);
            q.exec(QString("ALTER TABLE intervals ADD COLUMN %1 %2").arg(col.name).arg(col.def));
        }

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
            if (columnExists(db, "climbs", col.name)) continue;
            QSqlQuery q(db);
            q.exec(QString("ALTER TABLE climbs ADD COLUMN %1 %2").arg(col.name).arg(col.def));
        }

        {
            QSqlQuery q(db);
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
            QSqlQuery q(db);
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
        {
            QSqlQuery q(db);
            q.exec("CREATE INDEX IF NOT EXISTS idx_activity_samples_time"
                   " ON activity_samples(activity_id, elapsed_seconds)");
        }
        {
            QSqlQuery q(db);
            q.exec("CREATE INDEX IF NOT EXISTS idx_imports_date"
                   " ON imports(imported_at)");
        }
        {
            QSqlQuery q(db);
            q.exec("CREATE INDEX IF NOT EXISTS idx_workouts_athlete_date"
                   " ON planned_workouts(athlete_id, workout_date)");
        }
    }

    return applySchema(db, errorOut);
}
}
