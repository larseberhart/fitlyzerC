// SPDX-License-Identifier: GPL-3

/**
 * @file TileCache.h
 * @brief Map tile caching and retrieval.
 *
 * Provides memory and disk caching of map tiles and
 * manages tile downloads from configured providers.
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QCache>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPixmap>
#include <QQueue>
#include <QSet>
#include <QString>
#include <QStringList>
/**
 * @brief Map tile provider styles.
 */enum class MapStyle
{
    Street = 0,
    Light = 1,
    Dark = 4,
    Satellite = 5,
    Terrain = 6,
    Topographic = 7,
    Minimal = 8
};

/**
 * @brief Tile provider configuration.
 */
struct TileProvider
{
    /// @brief Provider name.
    QString name;

    /// @brief URL template with {z}/{x}/{y} placeholders.
    QString urlTemplate;

    /// @brief Maximum zoom level.
    int maxZoom = 18;

    /// @brief Attribution text for provider.
    QString attribution;
};

class QNetworkReply;

/**
 * @brief Configuration for tile caching behavior.
 */
struct TileCacheConfig
{
    /// @brief Maximum tiles to hold in memory.
    int maxTilesInMemory = 512;

    /// @brief Primary disk cache directory.
    QString diskCacheRoot;

    /// @brief Additional fallback cache directories.
    QStringList diskFallbackRoots;
};

/**
 * @brief Caches map tiles in memory and on disk.
 *
 * Provides tile lookup, network fetching, and memory/disk cache management
 * with multiple map style support and fallback cache locations.
 */
class TileCache : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Caches map tiles in memory and on disk.
     *
     * Provides tile lookup, network fetching, and memory/disk cache management
     * with multiple map style support and fallback cache locations.
     */
    /**
     * @brief Constructs tile cache with configuration.
     * @param config Cache configuration.
     * @param parent Parent object.
     */
    explicit TileCache(const TileCacheConfig& config = {}, QObject* parent = nullptr);

    /**
     * @brief Gets cached tile pixmap (may be null while downloading).
     * @param z Zoom level.
     * @param x Tile x coordinate.
     * @param y Tile y coordinate.
     * @return Tile pixmap or null if not cached.
     */
    QPixmap tile(int z, int x, int y);

    /**
     * @brief Gets tile pixmap, downloading if needed.
     * @param z Zoom level.
     * @param x Tile x coordinate.
     * @param y Tile y coordinate.
     * @param allowNetwork Whether to download if not cached.
     * @return Tile pixmap or null if unavailable.
     */
    QPixmap tileBlocking(int z, int x, int y, bool allowNetwork = true);

    /**
     * @brief Checks if tile is cached on disk.
     * @param z Zoom level.
     * @param x Tile x coordinate.
     * @param y Tile y coordinate.
     * @return True if tile is available on disk.
     */
    bool isTileCachedOnDisk(int z, int x, int y) const;

    /**
     * @brief Clears memory cache (disk cache preserved).
     */
    void clearMemoryCache();

    /**
     * @brief Resizes memory cache and drops excess entries.
     * @param maxTiles New maximum tiles in memory.
     */
    void setMemoryCacheSize(int maxTiles);

    /**
     * @brief Sets map style (updates tile provider).
     * @param style Map style.
     */
    void setMapStyle(MapStyle style);

    /**
     * @brief Gets current map style.
     * @return Current style.
     */
    MapStyle mapStyle() const { return m_mapStyle; }

    /**
     * @brief Gets display name for current map style.
     * @return Style display name.
     */
    QString mapStyleName() const;

    /**
     * @brief Gets attribution text for current provider.
     * @return Attribution string.
     */
    QString attribution() const;

    /**
     * @brief Gets maximum zoom level for current provider.
     * @return Maximum zoom.
     */
    int maxZoom() const;

    /**
     * @brief Gets disk cache root directory.
     * @return Cache directory path.
     */
    QString diskCacheRoot() const;

signals:
    /// \signal Emitted when a tile finishes loading.
    /// \param z Zoom level.
    /// \param x Tile X coordinate.
    /// \param y Tile Y coordinate.
    void tileLoaded(int z, int x, int y);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    struct TileRequest { int z, x, y; };

    static constexpr int kMaxConcurrentDownloads = 6;

    QString key(int z, int x, int y) const;
    static QString keyForStyle(const QString& style, int z, int x, int y);
    QString diskTilePathForRoot(const QString& root, int z, int x, int y) const;
    static QString diskTilePathForRoot(const QString& root,
                                       const QString& style,
                                       int z,
                                       int x,
                                       int y);
    QString diskTilePath(int z, int x, int y) const;
    static QString styleKey(MapStyle style);
    static QString styleDisplayName(MapStyle style);
    static TileProvider providerForStyle(MapStyle style);

    void dispatchNextDownloads();

    QNetworkAccessManager      m_nam;
    QCache<QString, QPixmap>   m_cache;
    QSet<QString>              m_pending;
    QQueue<TileRequest>        m_downloadQueue;
    int                        m_activeDownloads = 0;
    QString                    m_diskCacheRoot;
    QStringList                m_diskFallbackRoots;
    MapStyle                   m_mapStyle = MapStyle::Light;
    TileProvider               m_provider;
};