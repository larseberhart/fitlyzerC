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

struct TileProvider
{
    QString name;
    QString urlTemplate;
    int maxZoom = 18;
    QString attribution;
};

class QNetworkReply;

struct TileCacheConfig
{
    int maxTilesInMemory = 512;
    QString diskCacheRoot;
    QStringList diskFallbackRoots;
};

class TileCache : public QObject
{
    Q_OBJECT
public:
    explicit TileCache(const TileCacheConfig& config = {}, QObject* parent = nullptr);

    // Returns the cached tile pixmap (may be null while downloading)
    QPixmap tile(int z, int x, int y);
    QPixmap tileBlocking(int z, int x, int y, bool allowNetwork = true);
    bool isTileCachedOnDisk(int z, int x, int y) const;
    void clearMemoryCache();
    void setMemoryCacheSize(int maxTiles);  // live resize; drops excess entries
    void setMapStyle(MapStyle style);
    MapStyle mapStyle() const { return m_mapStyle; }
    QString mapStyleName() const;
    QString attribution() const;
    int maxZoom() const;
    QString diskCacheRoot() const;

signals:
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
