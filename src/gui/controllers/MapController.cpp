// SPDX-License-Identifier: GPL-3

#include "MapController.h"

#include "../WorkoutController.h"
#include "../../database/DatabaseManager.h"
#include "maps/MapRenderer.h"

/**
 * @brief Constructs the map controller.
 * @param controller Pointer to WorkoutController for data access.
 * @param dbManager Pointer to DatabaseManager for queries.
 * @param parent Parent object.
 */
MapController::MapController(
    WorkoutController* controller,
    DatabaseManager* dbManager,
    QObject* parent)
    : QObject(parent)
    , m_controller(controller)
    , m_dbManager(dbManager)
{
}

/**
 * @brief Destructor.
 */
MapController::~MapController() = default;

/**
 * @brief Sets the MapRenderer widget managed by this controller.
 * @param mapRenderer Pointer to MapRenderer widget.
 */
void MapController::setMapRenderer(MapRenderer* mapRenderer)
{
    m_mapRenderer = mapRenderer;
    connectMapSignals();
}

/**
 * @brief Gets the managed map renderer widget.
 * @return Pointer to MapRenderer, or nullptr if not set.
 */
MapRenderer* MapController::mapRenderer() const
{
    return m_mapRenderer;
}

/**
 * @brief Updates map display with current activity route data.
 */
void MapController::updateMap()
{
    // TODO: Extract map update logic from MainWindow
    emit mapUpdated();
}

/**
 * @brief Refreshes map rendering without reloading data.
 */
void MapController::refreshMap()
{
    // TODO: Extract map refresh logic from MainWindow
    emit mapUpdated();
}

/**
 * @brief Fits map view to entire track.
 */
void MapController::fitToTrack()
{
    if (!m_mapRenderer)
        return;
    m_mapRenderer->fitToTrack();
}

/**
 * @brief Fits map view to visible time range.
 */
void MapController::fitToVisibleRange()
{
    if (!m_mapRenderer)
        return;
    m_mapRenderer->fitToVisibleRange();
}

/**
 * @brief Zooms in on the map.
 */
void MapController::zoomIn()
{
    if (!m_mapRenderer)
        return;
    m_mapRenderer->zoomIn();
}

/**
 * @brief Zooms out on the map.
 */
void MapController::zoomOut()
{
    if (!m_mapRenderer)
        return;
    m_mapRenderer->zoomOut();
}

/**
 * @brief Sets the current crosshair time position.
 * @param timeSeconds Time in seconds (-1 to clear).
 */
void MapController::setCurrentTime(double timeSeconds)
{
    if (!m_mapRenderer)
        return;
    m_mapRenderer->setCurrentTime(timeSeconds);
}

/**
 * @brief Sets the visible time range on the map.
 * @param startSeconds Start time in seconds.
 * @param endSeconds End time in seconds.
 */
void MapController::setVisibleTimeRange(double startSeconds, double endSeconds)
{
    if (!m_mapRenderer)
        return;
    m_mapRenderer->setVisibleTimeRange(startSeconds, endSeconds);
}

/**
 * @brief Highlights a segment on the map.
 * @param startSeconds Segment start time in seconds.
 * @param endSeconds Segment end time in seconds.
 */
void MapController::highlightSegment(double startSeconds, double endSeconds)
{
    // TODO: Extract segment highlighting from MainWindow
}

/**
 * @brief Updates route coloring based on current metric.
 */
void MapController::updateRouteColoring()
{
    applyColorMetric();
}

/**
 * @brief Enables/disables automatic route contrast adjustment.
 * @param enabled Whether to enable auto contrast.
 */
void MapController::setAutoRouteContrast(bool enabled)
{
    if (!m_mapRenderer)
        return;
    m_mapRenderer->setAutoRouteContrast(enabled);
}

/**
 * @brief Helper to apply current color metric to map.
 */
void MapController::applyColorMetric()
{
    // TODO: Extract color metric application from MainWindow
}

/**
 * @brief Helper to connect map signals.
 */
void MapController::connectMapSignals()
{
    // TODO: Extract signal connections from MainWindow
}
