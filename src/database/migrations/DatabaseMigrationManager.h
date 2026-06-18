// SPDX-License-Identifier: GPL-3

#pragma once

#include <QString>

class QSqlDatabase;

namespace DatabaseMigrationManager
{
bool applySchema(QSqlDatabase& db, QString* errorOut);
bool upgradeSchema(QSqlDatabase& db, int fromVersion, QString* errorOut);
}
