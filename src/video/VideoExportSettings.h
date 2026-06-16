// SPDX-License-Identifier: GPL-3


#pragma once

#include <QColor>
#include <QString>

#include "core/zones/ZoneDefinition.h"
#include "fit/RideData.h"
#include "maps/TileCache.h"

/**
 * @brief Configuration for activity video export.
 *
 * Specifies output format, map rendering options, data overlays,
 * and telemetry display settings for video generation.
 */
struct VideoExportSettings
{
    /// @brief Output file path.
    QString outputPath;

    /// @brief FFmpeg executable path.
    QString ffmpegPath;

    /// @brief Output video width in pixels.
    int width = 1920;

    /// @brief Output video height in pixels.
    int height = 1080;

    /// @brief Output video frame rate in fps.
    int fps = 30;

    /// @brief Playback speed multiplier.
    double playbackSpeed = 3.0;

    /// @brief Whether to render only selected segment.
    bool useSelectedSegment = true;

    /// @brief Segment start time in seconds.
    double segmentStartSeconds = 0.0;

    /// @brief Segment end time in seconds.
    double segmentEndSeconds = 0.0;

    /// @brief Map style for rendering.
    MapStyle mapStyle = MapStyle::Light;

    /// @brief Whether to enable automatic zoom adjustment.
    bool autoZoom = true;

    /// @brief Fixed zoom level when autoZoom disabled.
    int fixedZoom = 18;

    /// @brief Whether to center map on athlete position.
    bool followAthlete = true;

    /// @brief Color metric for route coloring.
    ColorMetric routeColorMetric = ColorMetric::Power;

    /// @brief Fixed route color when not using metric coloring.
    QColor singleRouteColor = QColor("#2563eb");

    /// @brief Whether to overlay power data.
    bool overlayPower = true;

    /// @brief Whether to overlay heart rate data.
    bool overlayHeartRate = true;

    /// @brief Whether to overlay cadence data.
    bool overlayCadence = true;

    /// @brief Whether to overlay speed data.
    bool overlaySpeed = true;

    /// @brief Whether to overlay altitude data.
    bool overlayAltitude = true;

    /// @brief Whether to overlay elapsed time.
    bool overlayTime = true;

    /// @brief Whether to overlay athlete name.
    bool overlayAthleteName = true;

    /// @brief Whether to overlay telemetry panel.
    bool overlayTelemetryPanel = true;

    /// @brief Whether to overlay route legend.
    bool overlayRouteLegend = true;

    /// @brief Whether to overlay metric charts.
    bool overlayCharts = true;

    /// @brief Whether to delete temporary tiles after export.
    bool deleteTemporaryTilesAfterExport = true;

    /// @brief Athlete name for display.
    QString athleteName;

    /// @brief Activity name for display.
    QString activityName;

    /// @brief Color context for metric coloring.
    ColorContext colorContext;

    /// @brief Activity ride data for rendering.
    RideData rideData;
};