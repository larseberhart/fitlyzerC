// SPDX-License-Identifier: GPL-3


#pragma once

#include <QCache>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPixmap>
#include <QQueue>
#include <QSet>
#include <QString>
#include <QStringList>
enum class MapStyle
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
    // Provider name.
    QString name;

    // URL template with {z}/{x}/{y} placeholders.
    QString urlTemplate;

    // Maximum zoom level.
    int maxZoom = 18;

    // Attribution text for provider.
    QString attribution;
};

class QNetworkReply;

/**
 * @brief Configuration for tile caching behavior.
 */
struct TileCacheConfig
{
    // Maximum tiles to hold in memory.
    int maxTilesInMemory = 512;

    // Primary disk cache directory.
    QString diskCacheRoot;

    // Additional fallback cache directories.
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
    explicit TileCache(const TileCacheConfig& config = {}, QObject* parent = nullptr);

    QPixmap tile(int z, int x, int y);

    QPixmap tileBlocking(int z, int x, int y, bool allowNetwork = true);

    bool isTileCachedOnDisk(int z, int x, int y) const;

    void clearMemoryCache();

    void setMemoryCacheSize(int maxTiles);

    void setMapStyle(MapStyle style);

    MapStyle mapStyle() const { return m_mapStyle; }

    QString mapStyleName() const;

    QString attribution() const;

    int maxZoom() const;

    QString diskCacheRoot() const;

signals:
    // Emitted when a tile finishes loading.
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