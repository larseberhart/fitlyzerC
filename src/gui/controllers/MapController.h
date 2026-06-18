// SPDX-License-Identifier: GPL-3

#pragma once

#include <QObject>
#include <memory>

class WorkoutController;
class DatabaseManager;
class MapRenderer;

/**
 * @brief Manages map rendering and interaction.
 *
 * Handles:
 * - MapRenderer widget management
 * - Map styles (light, street, satellite, dark, terrain, topographic, minimal)
 * - Route coloring based on metrics
 * - Map refresh and synchronization
 * - Segment highlighting
 * - Zoom and pan operations
 */
class MapController : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the map controller.
     * @param controller Pointer to WorkoutController for data access.
     * @param dbManager Pointer to DatabaseManager for queries.
     * @param parent Parent object.
     */
    explicit MapController(
        WorkoutController* controller,
        DatabaseManager* dbManager,
        QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~MapController() override;

    /**
     * @brief Sets the MapRenderer widget managed by this controller.
     * @param mapRenderer Pointer to MapRenderer widget.
     */
    void setMapRenderer(MapRenderer* mapRenderer);

    /**
     * @brief Gets the managed map renderer widget.
     * @return Pointer to MapRenderer, or nullptr if not set.
     */
    MapRenderer* mapRenderer() const;

    /**
     * @brief Updates map display with current activity route data.
     */
    void updateMap();

    /**
     * @brief Refreshes map rendering without reloading data.
     */
    void refreshMap();

    /**
     * @brief Fits map view to entire track.
     */
    void fitToTrack();

    /**
     * @brief Fits map view to visible time range.
     */
    void fitToVisibleRange();

    /**
     * @brief Zooms in on the map.
     */
    void zoomIn();

    /**
     * @brief Zooms out on the map.
     */
    void zoomOut();

    /**
     * @brief Sets the current crosshair time position.
     * @param timeSeconds Time in seconds (-1 to clear).
     */
    void setCurrentTime(double timeSeconds);

    /**
     * @brief Sets the visible time range on the map.
     * @param startSeconds Start time in seconds.
     * @param endSeconds End time in seconds.
     */
    void setVisibleTimeRange(double startSeconds, double endSeconds);

    /**
     * @brief Highlights a segment on the map.
     * @param startSeconds Segment start time in seconds.
     * @param endSeconds Segment end time in seconds.
     */
    void highlightSegment(double startSeconds, double endSeconds);

    /**
     * @brief Updates route coloring based on current metric.
     */
    void updateRouteColoring();

    /**
     * @brief Enables/disables automatic route contrast adjustment.
     * @param enabled Whether to enable auto contrast.
     */
    void setAutoRouteContrast(bool enabled);

signals:
    /// @brief Emitted when map has been successfully updated.
    void mapUpdated();

    /// @brief Emitted when an error occurs during map operations.
    void errorOccurred(const QString& message);

private:
    /// @brief Helper to apply current color metric to map.
    void applyColorMetric();

    /// @brief Helper to connect map signals.
    void connectMapSignals();

    WorkoutController* m_controller;
    DatabaseManager* m_dbManager;
    MapRenderer* m_mapRenderer = nullptr;
};
