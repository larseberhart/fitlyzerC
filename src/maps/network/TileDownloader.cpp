// SPDX-License-Identifier: GPL-3

#include "TileDownloader.h"

#include <QUrl>

namespace TileDownloader
{
QString tileUrl(const QString& urlTemplate, int z, int x, int y)
{
    return QString(urlTemplate)
        .replace("{z}", QString::number(z))
        .replace("{x}", QString::number(x))
        .replace("{y}", QString::number(y));
}

QNetworkRequest buildRequest(const QString& urlTemplate, int z, int x, int y)
{
    return QNetworkRequest(QUrl(tileUrl(urlTemplate, z, x, y)));
}
}
