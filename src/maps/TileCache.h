#pragma once

#include <QCache>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPixmap>
#include <QSet>
#include <QString>

enum class MapStyle
{
    Street,
    Light,
    Greyscale,
    BlackWhite,
    Dark,
    Satellite,
    Terrain,
    Topographic,
    Minimal
};

struct TileProvider
{
    QString name;
    QString urlTemplate;
    int maxZoom = 18;
    QString attribution;
};

class QNetworkReply;

class TileCache : public QObject
{
    Q_OBJECT
public:
    explicit TileCache(QObject* parent = nullptr);

    // Returns the cached tile pixmap (may be null while downloading)
    QPixmap tile(int z, int x, int y);
    void setMapStyle(MapStyle style);
    MapStyle mapStyle() const { return m_mapStyle; }
    QString mapStyleName() const;
    QString attribution() const;
    int maxZoom() const;

signals:
    void tileLoaded(int z, int x, int y);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QString key(int z, int x, int y) const;
    static QString styleKey(MapStyle style);
    static QString styleDisplayName(MapStyle style);
    static TileProvider providerForStyle(MapStyle style);

    QNetworkAccessManager      m_nam;
    QCache<QString, QPixmap>   m_cache;
    QSet<QString>              m_pending;
    MapStyle                   m_mapStyle = MapStyle::Light;
    TileProvider               m_provider;
};
