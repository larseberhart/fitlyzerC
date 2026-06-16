// SPDX-License-Identifier: GPL-3


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