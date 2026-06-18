// SPDX-License-Identifier: GPL-3

#pragma once

#include <QPointF>

#include <vector>

#include "fit/RideData.h"
#include "video/VideoTileProvider.h"

namespace VideoRenderer
{
struct TileViewport
{
    int tileX0 = 0;
    int tileY0 = 0;
    int nX = 0;
    int nY = 0;

    bool operator==(const TileViewport& other) const
    {
        return tileX0 == other.tileX0 &&
               tileY0 == other.tileY0 &&
               nX == other.nX &&
               nY == other.nY;
    }
};

TileViewport computeTileViewport(const QPointF& centerTile, int width, int height);
std::vector<VideoTileId> tilesForViewport(const TileViewport& viewport, int zoom);
QPointF latLonToTile(double lat, double lon, int zoom);
QPointF tileToScreen(const QPointF& tilePt, const QPointF& centerTile, int width, int height);
int nearestRecordIndex(const std::vector<RideRecord>& records, double elapsedSeconds);
bool interpolateGpsAtTime(const std::vector<const RideRecord*>& gpsRecords,
                          double elapsedSeconds,
                          double& outLat,
                          double& outLon);
}
