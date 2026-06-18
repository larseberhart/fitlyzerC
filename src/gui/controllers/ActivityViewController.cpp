// SPDX-License-Identifier: GPL-3

#include "ActivityViewController.h"

#include "../WorkoutController.h"
#include "../../database/DatabaseManager.h"

/**
 * @brief Constructs the activity view controller.
 * @param controller Pointer to WorkoutController for data access.
 * @param dbManager Pointer to DatabaseManager for queries.
 * @param parent Parent object.
 */
ActivityViewController::ActivityViewController(
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
ActivityViewController::~ActivityViewController() = default;

/**
 * @brief Loads an activity by ID.
 * @param activityId ID of activity to load.
 * @param outError Output string for error message on failure.
 * @return True if successfully loaded, false otherwise.
 */
bool ActivityViewController::loadActivity(int activityId, QString& outError)
{
    if (!m_controller)
    {
        outError = "Activity controller not initialized.";
        return false;
    }

    if (!m_controller->loadActivity(activityId, outError))
        return false;

    updateActivityDisplay();
    emit activityLoaded(activityId);
    return true;
}

/**
 * @brief Refreshes the display of the currently loaded activity.
 */
void ActivityViewController::refreshActivity()
{
    updateActivityDisplay();
    emit activityDisplayUpdated();
}

/**
 * @brief Updates all activity-related UI elements.
 *
 * Called after loading a new activity or when data changes.
 */
void ActivityViewController::updateActivityDisplay()
{
    updateActivityMetadata();
    populateActivityDetails();
    syncAnalysisViews();
}

/**
 * @brief Helper to update activity metadata in UI.
 */
void ActivityViewController::updateActivityMetadata()
{
    // TODO: Extract metadata update logic from MainWindow
}

/**
 * @brief Helper to populate activity detail panels.
 */
void ActivityViewController::populateActivityDetails()
{
    // TODO: Extract detail panel population from MainWindow
}

/**
 * @brief Helper to sync charts and map with current activity.
 */
void ActivityViewController::syncAnalysisViews()
{
    // TODO: Extract analysis view synchronization from MainWindow
}
