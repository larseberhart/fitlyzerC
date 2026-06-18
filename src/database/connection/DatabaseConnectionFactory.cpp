// SPDX-License-Identifier: GPL-3

#include "DatabaseConnectionFactory.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace DatabaseConnectionFactory
{
QString createConnectionName()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool openSqlite(QSqlDatabase& db,
                const QString& path,
                const QString& connectionName,
                QString* errorOut)
{
    db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(path);

    if (db.open())
        return true;

    if (errorOut)
        *errorOut = db.lastError().text();
    QSqlDatabase::removeDatabase(connectionName);
    return false;
}

void applySqlitePragmas(QSqlDatabase& db)
{
    QSqlQuery q(db);
    q.exec("PRAGMA journal_mode=WAL");
    q.exec("PRAGMA synchronous=NORMAL");
    q.exec("PRAGMA temp_store=MEMORY");
    q.exec("PRAGMA foreign_keys=ON");
    q.exec("PRAGMA cache_size=-20000");
}
}
