// SPDX-License-Identifier: GPL-3

/**
 * @file MapFitMath.cpp
 * @brief Map rendering support for MapFitMath.
 *
 * Implements map math, tile caching, and rendering logic used for geographic visualization of activities.
 *
 * Responsibilities:
 * - Provide map computation, tile handling, or rendering functionality
 *
 * @author Lars EBERHART
 */

#include "MapFitMath.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr int kTilePx = 256;
constexpr double kMinLatSpan = 0.001;
constexpr double kMinLonSpan = 0.001;
constexpr double kMaxMercatorLat = 85.051129;

double normalizeLongitude(double lon)
{
    double normalized = std::fmod(lon + 180.0, 360.0);
    if (normalized < 0.0)
        normalized += 360.0;
    return normalized - 180.0;
}

double latToTileY(double lat)
{
    const double clamped = std::clamp(lat, -kMaxMercatorLat, kMaxMercatorLat);
    const double rad = clamped * kPi / 180.0;
    return (1.0 - std::log(std::tan(rad) + 1.0 / std::cos(rad)) / kPi) / 2.0;
}

double tileYToLat(double tileY)
{
    const double mercN = kPi * (1.0 - 2.0 * tileY);
    return std::atan(std::sinh(mercN)) * 180.0 / kPi;
}
}

namespace MapFitMath
{
FitResult computeFitToBounds(double minLat,
                             double maxLat,
                             double minLon,
                             double maxLon,
                             int viewportWidth,
                             int viewportHeight,
                             int maxProviderZoom)
{
    if (maxLat < minLat)
        std::swap(minLat, maxLat);
    if (maxLon < minLon)
        std::swap(minLon, maxLon);

    if (maxLat - minLat < kMinLatSpan)
    {
        const double center = (minLat + maxLat) / 2.0;
        minLat = center - kMinLatSpan / 2.0;
        maxLat = center + kMinLatSpan / 2.0;
    }
    if (maxLon - minLon < kMinLonSpan)
    {
        const double center = (minLon + maxLon) / 2.0;
        minLon = center - kMinLonSpan / 2.0;
        maxLon = center + kMinLonSpan / 2.0;
    }

    minLat = std::clamp(minLat, -kMaxMercatorLat, kMaxMercatorLat);
    maxLat = std::clamp(maxLat, -kMaxMercatorLat, kMaxMercatorLat);
    minLon = normalizeLongitude(minLon);
    maxLon = normalizeLongitude(maxLon);

    FitResult result;
    result.minLat = minLat;
    result.maxLat = maxLat;
    result.minLon = minLon;
    result.maxLon = maxLon;

    double lonSpan = result.maxLon - result.minLon;
    if (lonSpan < 0.0)
        lonSpan += 360.0;
    const bool wrapsDateline = lonSpan > 180.0;
    if (wrapsDateline)
        lonSpan = 360.0 - lonSpan;
    result.lonSpanDegrees = lonSpan;

    const double northTileY = latToTileY(result.maxLat);
    const double southTileY = latToTileY(result.minLat);
    const double centerTileY = (northTileY + southTileY) / 2.0;
    result.centerLat = std::clamp(tileYToLat(centerTileY), -kMaxMercatorLat, kMaxMercatorLat);

    if (wrapsDateline)
    {
        double shiftedMin = result.minLon;
        double shiftedMax = result.maxLon;
        if (shiftedMin < 0.0)
            shiftedMin += 360.0;
        if (shiftedMax < 0.0)
            shiftedMax += 360.0;
        if (shiftedMin > shiftedMax)
            std::swap(shiftedMin, shiftedMax);
        result.centerLon = normalizeLongitude((shiftedMin + shiftedMax) / 2.0);
    }
    else
    {
        result.centerLon = normalizeLongitude((result.minLon + result.maxLon) / 2.0);
    }

    const int w = std::max(viewportWidth, 800);
    const int h = std::max(viewportHeight, 400);

    if (result.maxLat <= result.minLat || result.lonSpanDegrees <= 0.0)
    {
        result.zoom = std::min(15, maxProviderZoom);
        return result;
    }

    const int cappedMaxZoom = std::clamp(maxProviderZoom, 1, 18);
    result.zoom = 1;
    for (int z = cappedMaxZoom; z >= 1; --z)
    {
        const double n = std::pow(2.0, z);
        const double pixW = (result.lonSpanDegrees / 360.0) * n * kTilePx;
        const double pixH = std::abs(southTileY - northTileY) * n * kTilePx;
        if (pixW <= w * 0.85 && pixH <= h * 0.85)
        {
            result.zoom = z;
            break;
        }
    }

    return result;
}
}