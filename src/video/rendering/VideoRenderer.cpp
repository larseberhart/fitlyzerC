// SPDX-License-Identifier: GPL-3

#include "VideoRenderer.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr int kTilePx = 256;
constexpr double kPi = 3.14159265358979323846;
}

namespace VideoRenderer
{
TileViewport computeTileViewport(const QPointF& centerTile, int width, int height)
{
    TileViewport viewport;
    viewport.tileX0 = static_cast<int>(std::floor(centerTile.x() - static_cast<double>(width) / (2.0 * kTilePx))) - 1;
    viewport.tileY0 = static_cast<int>(std::floor(centerTile.y() - static_cast<double>(height) / (2.0 * kTilePx))) - 1;
    viewport.nX = static_cast<int>(std::ceil(static_cast<double>(width) / kTilePx)) + 3;
    viewport.nY = static_cast<int>(std::ceil(static_cast<double>(height) / kTilePx)) + 3;
    return viewport;
}

std::vector<VideoTileId> tilesForViewport(const TileViewport& viewport, int zoom)
{
    std::vector<VideoTileId> tiles;
    const int maxTile = (1 << zoom) - 1;
    tiles.reserve(static_cast<size_t>((viewport.nX + 1) * (viewport.nY + 1)));

    for (int ty = 0; ty <= viewport.nY; ++ty)
    {
        for (int tx = 0; tx <= viewport.nX; ++tx)
        {
            const int tileX = viewport.tileX0 + tx;
            const int tileY = viewport.tileY0 + ty;
            if (tileX < 0 || tileY < 0 || tileX > maxTile || tileY > maxTile)
                continue;
            tiles.push_back({ zoom, tileX, tileY });
        }
    }

    return tiles;
}

QPointF latLonToTile(double lat, double lon, int zoom)
{
    const double n = std::pow(2.0, zoom);
    const double x = (lon + 180.0) / 360.0 * n;
    const double rad = lat * kPi / 180.0;
    const double y = (1.0 - std::log(std::tan(rad) + 1.0 / std::cos(rad)) / kPi) / 2.0 * n;
    return { x, y };
}

QPointF tileToScreen(const QPointF& tilePt, const QPointF& centerTile, int width, int height)
{
    return {
        width / 2.0 + (tilePt.x() - centerTile.x()) * kTilePx,
        height / 2.0 + (tilePt.y() - centerTile.y()) * kTilePx
    };
}
}
