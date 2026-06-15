// SPDX-License-Identifier: GPL-3

/**
 * @file Platform_linux.cpp
 * @brief Linux-specific filesystem and OS integration.
 *
 * Provides platform-specific paths, file manager integration,
 * and other Linux-specific functionality.
 *
 * @author Lars EBERHART
 */

#include "Platform.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>

#if defined(Q_OS_LINUX)

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