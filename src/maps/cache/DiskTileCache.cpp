// SPDX-License-Identifier: GPL-3

#include "DiskTileCache.h"

#include <QDir>
#include <QFileInfo>

namespace DiskTileCache
{
QString diskTilePathForRoot(const QString& root,
                            const QString& style,
                            int z,
                            int x,
                            int y)
{
    return QDir(root).filePath(QString("%1/%2/%3/%4.png")
                                   .arg(style)
                                   .arg(z)
                                   .arg(x)
                                   .arg(y));
}

bool isTileCachedOnDisk(const QString& primaryRoot,
                        const QStringList& fallbackRoots,
                        const QString& style,
                        int z,
                        int x,
                        int y)
{
    if (QFileInfo::exists(diskTilePathForRoot(primaryRoot, style, z, x, y)))
        return true;

    for (const QString& fallbackRoot : fallbackRoots)
    {
        if (fallbackRoot.trimmed().isEmpty())
            continue;
        if (QFileInfo::exists(diskTilePathForRoot(fallbackRoot, style, z, x, y)))
            return true;
    }

    return false;
}
}
