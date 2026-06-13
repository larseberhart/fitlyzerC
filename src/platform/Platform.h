#pragma once

#include <QString>

namespace Platform
{
QString applicationDataDirectory();
QString defaultDatabaseDirectory();
QString defaultDatabasePath();
QString defaultBackupPath(const QString& databasePath);
void openInFileManager(const QString& path);
}