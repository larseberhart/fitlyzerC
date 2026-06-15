// SPDX-License-Identifier: GPL-3

/**
 * @file TileCache.cpp
 * @brief Map rendering support for TileCache.
 *
 * Implements map math, tile caching, and rendering logic used for geographic visualization of activities.
 *
 * Responsibilities:
 * - Provide map computation, tile handling, or rendering functionality
 *
 * @author Lars EBERHART
 */

#include "TileCache.h"

#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QImage>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>

#include <algorithm>

namespace
{
QString defaultDiskCacheRoot()
{
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return appData.isEmpty()
        ? QDir::home().filePath(".fitlyzerc/tiles")
        : QDir(appData).filePath("tiles");
}
}

QString TileCache::styleKey(MapStyle style)
{
    switch (style)
    {
        case MapStyle::Street:      return "street";
        case MapStyle::Light:       return "light";
        case MapStyle::Dark:        return "dark";
        case MapStyle::Satellite:   return "satellite";
        case MapStyle::Terrain:     return "terrain";
        case MapStyle::Topographic: return "topographic";
        case MapStyle::Minimal:     return "minimal";
    }
    return "street";
}

QString TileCache::styleDisplayName(MapStyle style)
{
    switch (style)
    {
        case MapStyle::Street:      return "Street Map";
        case MapStyle::Light:       return "Light";
        case MapStyle::Dark:        return "Dark";
        case MapStyle::Satellite:   return "Satellite";
        case MapStyle::Terrain:     return "Terrain";
        case MapStyle::Topographic: return "Topographic";
        case MapStyle::Minimal:     return "Minimal";
    }
    return "Street Map";
}

TileProvider TileCache::providerForStyle(MapStyle style)
{
    switch (style)
    {
        case MapStyle::Street:
            return {
                "OpenStreetMap Standard",
                "https://tile.openstreetmap.org/{z}/{x}/{y}.png",
                19,
                "© OpenStreetMap contributors"
            };
        case MapStyle::Light:
            return {
                "Carto Positron",
                "https://a.basemaps.cartocdn.com/light_all/{z}/{x}/{y}.png",
                20,
                "© OpenStreetMap contributors © CARTO"
            };
        case MapStyle::Dark:
            return {
                "Carto Dark Matter",
                "https://a.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}.png",
                20,
                "© OpenStreetMap contributors © CARTO"
            };
        case MapStyle::Satellite:
            return {
                "Esri World Imagery",
                "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}",
                18,
                "Tiles © Esri"
            };
        case MapStyle::Terrain:
            return {
                "OpenTopoMap",
                "https://a.tile.opentopomap.org/{z}/{x}/{y}.png",
                17,
                "© OpenStreetMap contributors, SRTM | © OpenTopoMap"
            };
        case MapStyle::Topographic:
            return {
                "OpenTopoMap",
                "https://a.tile.opentopomap.org/{z}/{x}/{y}.png",
                17,
                "© OpenStreetMap contributors, SRTM | © OpenTopoMap"
            };
        case MapStyle::Minimal:
            return {
                "Carto Positron No Labels",
                "https://a.basemaps.cartocdn.com/light_nolabels/{z}/{x}/{y}.png",
                20,
                "© OpenStreetMap contributors © CARTO"
            };
    }

    return {
        "OpenStreetMap Standard",
        "https://tile.openstreetmap.org/{z}/{x}/{y}.png",
        19,
        "© OpenStreetMap contributors"
    };
}

TileCache::TileCache(const TileCacheConfig& config, QObject* parent)
    : QObject(parent)
    , m_cache(std::max(1, config.maxTilesInMemory))
    , m_diskCacheRoot(config.diskCacheRoot.trimmed().isEmpty()
          ? defaultDiskCacheRoot()
          : config.diskCacheRoot)
    , m_diskFallbackRoots(config.diskFallbackRoots)
{
    m_provider = providerForStyle(m_mapStyle);
    connect(&m_nam, &QNetworkAccessManager::finished,
            this,   &TileCache::onReplyFinished);
}

QString TileCache::key(int z, int x, int y) const
{
    return keyForStyle(styleKey(m_mapStyle), z, x, y);
}

QString TileCache::keyForStyle(const QString& style, int z, int x, int y)
{
    return QString("%1/%2/%3/%4").arg(style).arg(z).arg(x).arg(y);
}

QString TileCache::diskTilePathForRoot(const QString& root, int z, int x, int y) const
{
    return diskTilePathForRoot(root, styleKey(m_mapStyle), z, x, y);
}

QString TileCache::diskTilePathForRoot(const QString& root,
                                       const QString& style,
                                       int z,
                                       int x,
                                       int y)
{
    return QDir(root).filePath(QString("%1/%2/%3/%4.png")
                                   .arg(style)
                                   .arg(z)
                                   .arg(x)
                                   .arg(y));
}

QString TileCache::diskTilePath(int z, int x, int y) const
{
    return diskTilePathForRoot(m_diskCacheRoot, z, x, y);
}

bool TileCache::isTileCachedOnDisk(int z, int x, int y) const
{
    z = std::clamp(z, 1, m_provider.maxZoom);
    if (QFileInfo::exists(diskTilePath(z, x, y)))
        return true;

    for (const QString& fallbackRoot : m_diskFallbackRoots)
    {
        if (fallbackRoot.trimmed().isEmpty())
            continue;
        if (QFileInfo::exists(diskTilePathForRoot(fallbackRoot, z, x, y)))
            return true;
    }

    return false;
}

void TileCache::clearMemoryCache()
{
    m_cache.clear();
    m_pending.clear();
    m_downloadQueue.clear();
    // m_activeDownloads is not reset here: in-flight replies will still arrive
    // and decrement it correctly. dispatchNextDownloads() will be a no-op since
    // the queue is now empty.
}

void TileCache::setMemoryCacheSize(int maxTiles)
{
    m_cache.setMaxCost(std::max(1, maxTiles));
}

void TileCache::setMapStyle(MapStyle style)
{
    if (m_mapStyle == style)
        return;

    // Cancel in-flight downloads from the previous style to reduce stale work.
    const auto activeReplies = m_nam.findChildren<QNetworkReply*>();
    for (QNetworkReply* reply : activeReplies)
    {
        if (reply)
        {
            reply->setProperty("styleChangeAbort", true);
            reply->abort();
        }
    }

    m_mapStyle = style;
    m_provider = providerForStyle(style);
    clearMemoryCache();
}

QString TileCache::mapStyleName() const
{
    return styleDisplayName(m_mapStyle);
}

QString TileCache::attribution() const
{
    return m_provider.attribution;
}

int TileCache::maxZoom() const
{
    return m_provider.maxZoom;
}

QString TileCache::diskCacheRoot() const
{
    return m_diskCacheRoot;
}

QPixmap TileCache::tile(int z, int x, int y)
{
    z = std::clamp(z, 1, m_provider.maxZoom);
    const QString k = key(z, x, y);

    if (auto* px = m_cache.object(k))
        return *px;

    const QString diskPath = diskTilePath(z, x, y);
    QPixmap diskPixmap;
    if (diskPixmap.load(diskPath))
    {
        m_cache.insert(k, new QPixmap(diskPixmap));
        return diskPixmap;
    }

    for (const QString& fallbackRoot : m_diskFallbackRoots)
    {
        if (fallbackRoot.trimmed().isEmpty())
            continue;

        const QString fallbackPath = diskTilePathForRoot(fallbackRoot, z, x, y);
        if (!diskPixmap.load(fallbackPath))
            continue;

        const QFileInfo fi(diskPath);
        QDir().mkpath(fi.path());
        diskPixmap.save(diskPath);
        m_cache.insert(k, new QPixmap(diskPixmap));
        return diskPixmap;
    }

    if (!m_pending.contains(k))
    {
        m_pending.insert(k);
        m_downloadQueue.enqueue({z, x, y});
        dispatchNextDownloads();
    }

    return {};   // empty until the download completes
}

QPixmap TileCache::tileBlocking(int z, int x, int y, bool allowNetwork)
{
    z = std::clamp(z, 1, m_provider.maxZoom);
    const QString k = key(z, x, y);

    if (auto* px = m_cache.object(k))
        return *px;

    const QString diskPath = diskTilePath(z, x, y);
    QPixmap diskPixmap;
    if (diskPixmap.load(diskPath))
    {
        m_cache.insert(k, new QPixmap(diskPixmap));
        return diskPixmap;
    }

    for (const QString& fallbackRoot : m_diskFallbackRoots)
    {
        if (fallbackRoot.trimmed().isEmpty())
            continue;

        const QString fallbackPath = diskTilePathForRoot(fallbackRoot, z, x, y);
        if (!diskPixmap.load(fallbackPath))
            continue;

        const QFileInfo fi(diskPath);
        QDir().mkpath(fi.path());
        diskPixmap.save(diskPath);
        m_cache.insert(k, new QPixmap(diskPixmap));
        return diskPixmap;
    }

    if (!allowNetwork)
        return {};

    QString urlStr = m_provider.urlTemplate;
    urlStr.replace("{z}", QString::number(z));
    urlStr.replace("{x}", QString::number(x));
    urlStr.replace("{y}", QString::number(y));

    QNetworkAccessManager nam;
    QNetworkRequest req{ QUrl(urlStr) };
    req.setHeader(QNetworkRequest::UserAgentHeader, "FitlyzerC/1.0");
    req.setAttribute(
        QNetworkRequest::RedirectPolicyAttribute,
        QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = nam.get(req);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QPixmap px;
    if (reply->error() == QNetworkReply::NoError)
    {
        const QByteArray bytes = reply->readAll();
        if (px.loadFromData(bytes))
        {
            const QFileInfo fi(diskPath);
            QDir().mkpath(fi.path());
            px.save(diskPath);
            m_cache.insert(k, new QPixmap(px));
        }
    }

    reply->deleteLater();
    return px;
}

void TileCache::dispatchNextDownloads()
{
    while (m_activeDownloads < kMaxConcurrentDownloads && !m_downloadQueue.isEmpty())
    {
        const TileRequest req = m_downloadQueue.dequeue();

        QString urlStr = m_provider.urlTemplate;
        urlStr.replace("{z}", QString::number(req.z));
        urlStr.replace("{x}", QString::number(req.x));
        urlStr.replace("{y}", QString::number(req.y));

        QNetworkRequest netReq{QUrl(urlStr)};
        netReq.setHeader(QNetworkRequest::UserAgentHeader, "FitlyzerC/1.0");
        netReq.setAttribute(
            QNetworkRequest::RedirectPolicyAttribute,
            QNetworkRequest::NoLessSafeRedirectPolicy);

        auto* reply = m_nam.get(netReq);
        reply->setProperty("z", req.z);
        reply->setProperty("x", req.x);
        reply->setProperty("y", req.y);
        reply->setProperty("style", styleKey(m_mapStyle));

        ++m_activeDownloads;
    }
}

void TileCache::onReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    
    // Safely decrement active downloads counter. Aborted replies or any other
    // completion will trigger this, so guard against going negative.
    if (m_activeDownloads > 0)
        --m_activeDownloads;

    const int z = reply->property("z").toInt();
    const int x = reply->property("x").toInt();
    const int y = reply->property("y").toInt();
    const QString replyStyle = reply->property("style").toString();
    const bool wasAbortedForStyleChange = reply->property("styleChangeAbort").toBool();
    const QString k = keyForStyle(replyStyle, z, x, y);
    m_pending.remove(k);

    // Always redispatch so queued tiles are sent as slots free up.
    dispatchNextDownloads();

    // Skip processing aborted or stale-style replies
    if (wasAbortedForStyleChange || replyStyle != styleKey(m_mapStyle))
        return;

    if (reply->error() != QNetworkReply::NoError)
        return;

    QPixmap px;
    if (!px.loadFromData(reply->readAll()))
        return;

    const QString diskPath = diskTilePathForRoot(m_diskCacheRoot, replyStyle, z, x, y);
    const QFileInfo fi(diskPath);
    QDir().mkpath(fi.path());
    px.save(diskPath);

    m_cache.insert(k, new QPixmap(px));
    emit tileLoaded(z, x, y);
}