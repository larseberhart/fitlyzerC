// SPDX-License-Identifier: GPL-3

#pragma once

#include <QString>

class QSqlDatabase;

namespace DatabaseConnectionFactory
{
QString createConnectionName();
bool openSqlite(QSqlDatabase& db,
                const QString& path,
                const QString& connectionName,
                QString* errorOut);
void applySqlitePragmas(QSqlDatabase& db);
}
