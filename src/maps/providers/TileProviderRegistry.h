// SPDX-License-Identifier: GPL-3

#pragma once

#include <QString>

#include "maps/TileCache.h"

namespace TileProviderRegistry
{
QString styleKey(MapStyle style);
QString styleDisplayName(MapStyle style);
TileProvider providerForStyle(MapStyle style);
}
