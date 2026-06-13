#include "TileCache.h"

#include <QImage>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

static constexpr int kCacheMaxTiles = 512;

QString TileCache::styleKey(MapStyle style)
{
    switch (style)
    {
        case MapStyle::Street:      return "street";
        case MapStyle::Light:       return "light";
        case MapStyle::Greyscale:   return "greyscale";
        case MapStyle::BlackWhite:  return "blackwhite";
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
        case MapStyle::Greyscale:   return "Greyscale";
        case MapStyle::BlackWhite:  return "Black & White";
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
        case MapStyle::Greyscale:
            return {
                "Wikimedia B/W",
                "https://tiles.wmflabs.org/bw-mapnik/{z}/{x}/{y}.png",
                18,
                "© OpenStreetMap contributors"
            };
        case MapStyle::BlackWhite:
            return {
                "Wikimedia B/W",
                "https://tiles.wmflabs.org/bw-mapnik/{z}/{x}/{y}.png",
                18,
                "© OpenStreetMap contributors"
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

TileCache::TileCache(QObject* parent)
    : QObject(parent)
    , m_cache(kCacheMaxTiles)
{
    m_provider = providerForStyle(m_mapStyle);
    connect(&m_nam, &QNetworkAccessManager::finished,
            this,   &TileCache::onReplyFinished);
}

QString TileCache::key(int z, int x, int y) const
{
    return QString("%1/%2/%3/%4").arg(styleKey(m_mapStyle)).arg(z).arg(x).arg(y);
}

void TileCache::setMapStyle(MapStyle style)
{
    if (m_mapStyle == style)
        return;

    m_mapStyle = style;
    m_provider = providerForStyle(style);
    m_cache.clear();
    m_pending.clear();
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

QPixmap TileCache::tile(int z, int x, int y)
{
    z = std::clamp(z, 1, m_provider.maxZoom);
    const QString k = key(z, x, y);

    if (auto* px = m_cache.object(k))
        return *px;

    if (!m_pending.contains(k))
    {
        m_pending.insert(k);

        const QString urlStr = m_provider.urlTemplate
            .replace("{z}", QString::number(z))
            .replace("{x}", QString::number(x))
            .replace("{y}", QString::number(y));
        const QUrl url(urlStr);

        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::UserAgentHeader,
                      "FitlyzerC/1.0");
        req.setAttribute(
            QNetworkRequest::RedirectPolicyAttribute,
            QNetworkRequest::NoLessSafeRedirectPolicy);

        auto* reply = m_nam.get(req);
        reply->setProperty("z", z);
        reply->setProperty("x", x);
        reply->setProperty("y", y);
        reply->setProperty("style", styleKey(m_mapStyle));
    }

    return {};   // empty until the download completes
}

void TileCache::onReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    const int z = reply->property("z").toInt();
    const int x = reply->property("x").toInt();
    const int y = reply->property("y").toInt();
    const QString style = reply->property("style").toString();
    const QString k = key(z, x, y);
    m_pending.remove(k);

    if (style != styleKey(m_mapStyle))
        return;

    if (reply->error() != QNetworkReply::NoError)
        return;

    QPixmap px;
    if (!px.loadFromData(reply->readAll()))
        return;

    m_cache.insert(k, new QPixmap(px));
    emit tileLoaded(z, x, y);
}
