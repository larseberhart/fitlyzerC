#include "Platform.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace Platform
{
QString applicationDataDirectory()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        dir = QDir::homePath() + "/FitlyzerC";

    QDir().mkpath(dir);
    return dir;
}

QString defaultDatabaseDirectory()
{
    const QString documentsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (!documentsDir.isEmpty())
        return documentsDir;

    return applicationDataDirectory();
}

QString defaultDatabasePath()
{
    return QDir(defaultDatabaseDirectory()).filePath("fitlyzer.fitlyzerdb");
}

QString defaultBackupPath(const QString& databasePath)
{
    if (databasePath.isEmpty())
        return QDir(defaultDatabaseDirectory()).filePath("fitlyzer-backup.fitlyzerdb");

    const QFileInfo info(databasePath);
    const QString baseName = info.completeBaseName().isEmpty()
        ? QStringLiteral("fitlyzer")
        : info.completeBaseName();
    return info.dir().filePath(baseName + "-backup.fitlyzerdb");
}
}