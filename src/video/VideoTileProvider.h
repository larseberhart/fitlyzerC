#pragma once

#include <QString>
#include <unordered_set>
#include <vector>

#include "maps/TileCache.h"

struct VideoTileProviderConfig
{
    MapStyle style = MapStyle::Light;
    int memoryCacheTiles = 96;
    bool deleteTemporaryTilesAfterExport = true;
    QString exportCacheRoot;
};

struct VideoTileId
{
    int z = 0;
    int x = 0;
    int y = 0;

    bool operator==(const VideoTileId& other) const
    {
        return z == other.z && x == other.x && y == other.y;
    }
};

struct VideoTileIdHash
{
    std::size_t operator()(const VideoTileId& id) const noexcept
    {
        const std::size_t hz = static_cast<std::size_t>(id.z);
        const std::size_t hx = static_cast<std::size_t>(id.x);
        const std::size_t hy = static_cast<std::size_t>(id.y);
        return (hz * 73856093u) ^ (hx * 19349663u) ^ (hy * 83492791u);
    }
};

struct VideoTileStats
{
    int requiredTiles = 0;
    int onDiskTiles = 0;
    int downloadedTiles = 0;
};

class VideoTileProvider
{
public:
    explicit VideoTileProvider(const VideoTileProviderConfig& config);

    void setRequiredTiles(const std::unordered_set<VideoTileId, VideoTileIdHash>& required);
    int requiredTileCount() const;
    int requiredTilesAvailableOnDisk() const;

    QPixmap getTile(const VideoTileId& id, bool allowNetwork);
    bool isTileCachedOnDisk(const VideoTileId& id) const;
    VideoTileStats stats() const;

    void clearMemoryCache();
    void cleanupExportCache();
    bool shouldDeleteExportCacheAfterExport() const;
    QString exportCacheRoot() const;

private:
    TileCache m_tileCache;
    bool m_deleteTemporaryTilesAfterExport = true;
    std::unordered_set<VideoTileId, VideoTileIdHash> m_requiredTiles;
    int m_requiredTilesOnDisk = 0;
    int m_downloadedTiles = 0;
};
