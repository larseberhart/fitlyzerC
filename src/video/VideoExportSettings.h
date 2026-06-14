#pragma once

#include <QColor>
#include <QString>

#include "core/zones/ZoneDefinition.h"
#include "fit/RideData.h"
#include "maps/TileCache.h"

struct VideoExportSettings
{
    QString outputPath;
    QString ffmpegPath;

    int width = 1920;
    int height = 1080;
    int fps = 30;
    double playbackSpeed = 3.0;

    bool useSelectedSegment = true;
    double segmentStartSeconds = 0.0;
    double segmentEndSeconds = 0.0;

    MapStyle mapStyle = MapStyle::Light;
    bool autoZoom = true;
    int fixedZoom = 18;
    bool followAthlete = true;

    ColorMetric routeColorMetric = ColorMetric::Power;
    QColor singleRouteColor = QColor("#2563eb");

    bool overlayPower = true;
    bool overlayHeartRate = true;
    bool overlayCadence = true;
    bool overlaySpeed = true;
    bool overlayAltitude = true;
    bool overlayTime = true;
    bool overlayAthleteName = true;
    bool overlayTelemetryPanel = true;
    bool overlayRouteLegend = true;
    bool overlayCharts = true;
    bool deleteTemporaryTilesAfterExport = true;

    QString athleteName;
    QString activityName;

    ColorContext colorContext;
    RideData rideData;
};
