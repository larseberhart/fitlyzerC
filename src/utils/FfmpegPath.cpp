#include "FfmpegPath.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QStandardPaths>

namespace
{
QString absoluteExecutablePath(const QString& candidate)
{
    if (candidate.isEmpty())
        return {};

    const QFileInfo info(candidate);
    if (info.exists() && info.isFile())
        return info.absoluteFilePath();

    return {};
}
}

QString resolveFfmpegExecutablePath()
{
#ifdef Q_OS_WIN
    const QString bundledWindowsPath = absoluteExecutablePath(
        QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"));
    if (!bundledWindowsPath.isEmpty())
        return bundledWindowsPath;
#endif

#ifdef FITLYZER_FFMPEG_PATH
    const QString configured = QString::fromUtf8(FITLYZER_FFMPEG_PATH);
    const QStringList candidates = {
        configured,
        QDir(QCoreApplication::applicationDirPath()).filePath(configured),
        QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),
        QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg"),
        QDir(QCoreApplication::applicationDirPath()).filePath("../Resources/ffmpeg"),
        QDir(QCoreApplication::applicationDirPath()).filePath("../Resources/ffmpeg.exe")
    };

    for (const QString& candidate : candidates)
    {
        const QString resolved = absoluteExecutablePath(candidate);
        if (!resolved.isEmpty())
            return resolved;
    }
#endif

    const QString systemFfmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
    if (!systemFfmpeg.isEmpty())
        return systemFfmpeg;

    return QStringLiteral("ffmpeg");
}