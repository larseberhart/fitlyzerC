#pragma once

namespace MapFitMath
{
struct FitResult
{
    double minLat = 0.0;
    double maxLat = 0.0;
    double minLon = 0.0;
    double maxLon = 0.0;
    double centerLat = 0.0;
    double centerLon = 0.0;
    double lonSpanDegrees = 0.0;
    int zoom = 1;
};

FitResult computeFitToBounds(double minLat,
                             double maxLat,
                             double minLon,
                             double maxLon,
                             int viewportWidth,
                             int viewportHeight,
                             int maxProviderZoom);
}
