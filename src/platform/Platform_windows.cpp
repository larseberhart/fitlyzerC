// SPDX-License-Identifier: GPL-3

/**
 * @file Platform_windows.cpp
 * @brief Windows-specific filesystem and OS integration.
 *
 * Provides platform-specific paths, file manager integration,
 * and other Windows-specific functionality.
 *
 * @author Lars EBERHART
 */

#include "Platform.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>

#if defined(Q_OS_WIN)

namespace Platform
{
void openInFileManager(const QString& path)
{
    const QFileInfo info(path);
    const QString target = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(target));
}
}

#endif