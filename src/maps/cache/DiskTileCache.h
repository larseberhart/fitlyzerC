// SPDX-License-Identifier: GPL-3

#pragma once

#include <QString>
#include <QStringList>

namespace DiskTileCache
{
QString diskTilePathForRoot(const QString& root,
                            const QString& style,
                            int z,
                            int x,
                            int y);

bool isTileCachedOnDisk(const QString& primaryRoot,
                        const QStringList& fallbackRoots,
                        const QString& style,
                        int z,
                        int x,
                        int y);
}
