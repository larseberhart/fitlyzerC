// SPDX-License-Identifier: GPL-3

/**
 * @file VideoTileProvider.cpp
 * @brief Video export and rendering component for VideoTileProvider.
 *
 * Implements video rendering, tile provision, and export settings used for activity video generation.
 *
 * Responsibilities:
 * - Provide video rendering or export-related functionality
 *
 * @author Lars EBERHART
 */

#include "VideoTileProvider.h"

#include <QDir>
#include <QStandardPaths>

#include <algorithm>

namespace
{
QString defaultVideoExportCacheRoot()
{
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return appData.isEmpty()
        ? QDir::home().filePath(".fitlyzerc/video-export-cache")
        : QDir(appData).filePath("video-export-cache");
}

QString defaultPrimaryTileCacheRoot()
{
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return appData.isEmpty()
        ? QDir::home().filePath(".fitlyzerc/tiles")
        : QDir(appData).filePath("tiles");
}
}

VideoTileProvider::VideoTileProvider(const VideoTileProviderConfig& config)
    : m_tileCache(
          TileCacheConfig{
              std::max(1, config.memoryCacheTiles),
              config.exportCacheRoot.trimmed().isEmpty()
                  ? defaultVideoExportCacheRoot()
                  : config.exportCacheRoot,
              QStringList{ defaultPrimaryTileCacheRoot() }
          })
    , m_deleteTemporaryTilesAfterExport(config.deleteTemporaryTilesAfterExport)
{
    m_tileCache.setMapStyle(config.style);
}

void VideoTileProvider::setRequiredTiles(const std::unordered_set<VideoTileId, VideoTileIdHash>& required)
{
    m_requiredTiles = required;
    m_requiredTilesOnDisk = 0;
    m_downloadedTiles = 0;

    for (const VideoTileId& tileId : m_requiredTiles)
    {
        if (m_tileCache.isTileCachedOnDisk(tileId.z, tileId.x, tileId.y))
            ++m_requiredTilesOnDisk;
    }
}

int VideoTileProvider::requiredTileCount() const
{
    return static_cast<int>(m_requiredTiles.size());
}

int VideoTileProvider::requiredTilesAvailableOnDisk() const
{
    return m_requiredTilesOnDisk;
}

QPixmap VideoTileProvider::getTile(const VideoTileId& id, bool allowNetwork)
{
    const bool cachedBefore = isTileCachedOnDisk(id);
    const QPixmap px = m_tileCache.tileBlocking(id.z, id.x, id.y, allowNetwork);
    if (allowNetwork && !cachedBefore && !px.isNull() && isTileCachedOnDisk(id))
        ++m_downloadedTiles;
    return px;
}

VideoTileStats VideoTileProvider::stats() const
{
    VideoTileStats s;
    s.requiredTiles = static_cast<int>(m_requiredTiles.size());
    s.onDiskTiles = m_requiredTilesOnDisk;
    s.downloadedTiles = m_downloadedTiles;
    return s;
}

bool VideoTileProvider::isTileCachedOnDisk(const VideoTileId& id) const
{
    return m_tileCache.isTileCachedOnDisk(id.z, id.x, id.y);
}

void VideoTileProvider::clearMemoryCache()
{
    m_tileCache.clearMemoryCache();
}

void VideoTileProvider::cleanupExportCache()
{
    if (!m_deleteTemporaryTilesAfterExport)
        return;

    QDir cacheRoot(m_tileCache.diskCacheRoot());
    if (cacheRoot.exists())
        cacheRoot.removeRecursively();
}

bool VideoTileProvider::shouldDeleteExportCacheAfterExport() const
{
    return m_deleteTemporaryTilesAfterExport;
}

QString VideoTileProvider::exportCacheRoot() const
{
    return m_tileCache.diskCacheRoot();
}