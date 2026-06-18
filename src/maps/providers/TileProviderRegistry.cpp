// SPDX-License-Identifier: GPL-3

#include "TileProviderRegistry.h"

namespace TileProviderRegistry
{
QString styleKey(MapStyle style)
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

QString styleDisplayName(MapStyle style)
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

TileProvider providerForStyle(MapStyle style)
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
}
