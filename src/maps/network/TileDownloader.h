// SPDX-License-Identifier: GPL-3

#pragma once

#include <QNetworkRequest>
#include <QString>

namespace TileDownloader
{
QString tileUrl(const QString& urlTemplate, int z, int x, int y);
QNetworkRequest buildRequest(const QString& urlTemplate, int z, int x, int y);
}
