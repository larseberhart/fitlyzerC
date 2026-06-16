// SPDX-License-Identifier: GPL-3


#pragma once

#include <QString>
#include <unordered_set>
#include <vector>

#include "maps/TileCache.h"

/**
 * @brief Configuration for video tile provider.
 */
struct VideoTileProviderConfig
{
    /// @brief Map style for video.
    MapStyle style = MapStyle::Light;

    /// @brief Maximum tiles to keep in memory.
    int memoryCacheTiles = 96;

    /// @brief Whether to delete temporary tiles after export.
    bool deleteTemporaryTilesAfterExport = true;

    /// @brief Root directory for export cache.
    QString exportCacheRoot;
};

/**
 * @brief Unique identifier for a map tile (z/x/y coordinates).
 */
struct VideoTileId
{
    /// @brief Zoom level.
    int z = 0;

    /// @brief X coordinate.
    int x = 0;

    /// @brief Y coordinate.
    int y = 0;

    /// @brief Equality comparison operator.
    bool operator==(const VideoTileId& other) const
    {
        return z == other.z && x == other.x && y == other.y;
    }
};

/**
 * @brief Hash function for VideoTileId.
 */
struct VideoTileIdHash
{
    /// @brief Computes hash for tile ID.
    std::size_t operator()(const VideoTileId& id) const noexcept
    {
        const std::size_t hz = static_cast<std::size_t>(id.z);
        const std::size_t hx = static_cast<std::size_t>(id.x);
        const std::size_t hy = static_cast<std::size_t>(id.y);
        return (hz * 73856093u) ^ (hx * 19349663u) ^ (hy * 83492791u);
    }
};

/**
 * @brief Tile availability statistics for export.
 */
struct VideoTileStats
{
    /// @brief Number of tiles required for video.
    int requiredTiles = 0;

    /// @brief Number of tiles available on disk.
    int onDiskTiles = 0;

    /// @brief Number of tiles downloaded during export prep.
    int downloadedTiles = 0;
};

/**
 * @brief Manages map tiles for video export.
 *
 * Handles tile caching, download coordination, and cleanup for video rendering.
 */
class VideoTileProvider
{
public:
    /**
     * @brief Constructs tile provider with configuration.
     * @param config Provider configuration.
     */
    explicit VideoTileProvider(const VideoTileProviderConfig& config);

    /**
     * @brief Sets the set of tiles required for video export.
     * @param required Set of required tile IDs.
     */
    void setRequiredTiles(const std::unordered_set<VideoTileId, VideoTileIdHash>& required);

    /**
     * @brief Gets count of required tiles.
     * @return Number of required tiles.
     */
    int requiredTileCount() const;

    /**
     * @brief Gets count of required tiles available on disk.
     * @return Number of tiles on disk cache.
     */
    int requiredTilesAvailableOnDisk() const;

    /**
     * @brief Retrieves a tile for rendering.
     * @param id Tile identifier.
     * @param allowNetwork True to fetch from network if not cached.
     * @return Tile pixmap, or empty if unavailable.
     */
    QPixmap getTile(const VideoTileId& id, bool allowNetwork);

    /**
     * @brief Checks if a tile is cached on disk.
     * @param id Tile identifier.
     * @return True if tile exists in disk cache.
     */
    bool isTileCachedOnDisk(const VideoTileId& id) const;

    /**
     * @brief Gets cache statistics.
     * @return Stats including required, on-disk, and downloaded counts.
     */
    VideoTileStats stats() const;

    /**
     * @brief Clears in-memory tile cache.
     */
    void clearMemoryCache();

    /**
     * @brief Cleans up export cache directory.
     */
    void cleanupExportCache();

    /**
     * @brief Checks if export cache should be deleted after export.
     * @return True if temporary tiles should be cleaned up.
     */
    bool shouldDeleteExportCacheAfterExport() const;

    /**
     * @brief Gets export cache root directory.
     * @return Cache directory path.
     */
    QString exportCacheRoot() const;

private:
    TileCache m_tileCache;
    bool m_deleteTemporaryTilesAfterExport = true;
    std::unordered_set<VideoTileId, VideoTileIdHash> m_requiredTiles;
    int m_requiredTilesOnDisk = 0;
    int m_downloadedTiles = 0;
};