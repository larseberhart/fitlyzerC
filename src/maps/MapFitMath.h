// SPDX-License-Identifier: GPL-3

/**
 * @file MapFitMath.h
 * @brief Map rendering support for MapFitMath.
 *
 * Implements map math, tile caching, and rendering logic used for geographic visualization of activities.
 *
 * Responsibilities:
 * - Provide map computation, tile handling, or rendering functionality
 *
 * @author Lars EBERHART
 */

#pragma once

namespace MapFitMath
{
/**
 * @brief Result of fitting GPS bounds to map viewport.
 */
struct FitResult
{
    /// @brief Minimum latitude of track bounds.
    double minLat = 0.0;

    /// @brief Maximum latitude of track bounds.
    double maxLat = 0.0;

    /// @brief Minimum longitude of track bounds.
    double minLon = 0.0;

    /// @brief Maximum longitude of track bounds.
    double maxLon = 0.0;

    /// @brief Center latitude of track bounds.
    double centerLat = 0.0;

    /// @brief Center longitude of track bounds.
    double centerLon = 0.0;

    /// @brief Longitude span in degrees for centering calculation.
    double lonSpanDegrees = 0.0;

    /// @brief Recommended zoom level to fit bounds in viewport.
    int zoom = 1;
};

/**
 * @brief Computes map zoom level and center to fit GPS bounds in viewport.
 * @param minLat Minimum latitude.
 * @param maxLat Maximum latitude.
 * @param minLon Minimum longitude.
 * @param maxLon Maximum longitude.
 * @param viewportWidth Viewport width in pixels.
 * @param viewportHeight Viewport height in pixels.
 * @param maxProviderZoom Maximum zoom level supported by tile provider.
 * @return Fit result with zoom and center coordinates.
 */
FitResult computeFitToBounds(double minLat,
                             double maxLat,
                             double minLon,
                             double maxLon,
                             int viewportWidth,
                             int viewportHeight,
                             int maxProviderZoom);
}