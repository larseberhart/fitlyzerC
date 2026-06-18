// SPDX-License-Identifier: GPL-3

#include "ImportController.h"

#include "../WorkoutController.h"
#include "../../database/DatabaseManager.h"
#include "import/ImportQueue.h"
#include "import/ImportProgressModel.h"
#include "../ImportStatusWidget.h"

/**
 * @brief Constructs the import controller.
 * @param controller Pointer to WorkoutController for data access.
 * @param dbManager Pointer to DatabaseManager for import destination.
 * @param parent Parent object.
 */
ImportController::ImportController(
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
ImportController::~ImportController() = default;

/**
 * @brief Sets the import queue.
 * @param importQueue Pointer to ImportQueue instance.
 */
void ImportController::setImportQueue(ImportQueue* importQueue)
{
    m_importQueue = importQueue;
}

/**
 * @brief Sets the import progress model.
 * @param progressModel Pointer to ImportProgressModel instance.
 */
void ImportController::setProgressModel(ImportProgressModel* progressModel)
{
    m_progressModel = progressModel;
}

/**
 * @brief Sets the import status widget.
 * @param statusWidget Pointer to ImportStatusWidget instance.
 */
void ImportController::setStatusWidget(ImportStatusWidget* statusWidget)
{
    m_statusWidget = statusWidget;
}

/**
 * @brief Queues a batch of files for import.
 * @param filePaths List of FIT file paths to import.
 * @param athleteId ID of target athlete.
 * @param showResultDialog Whether to show import result dialog.
 * @param sourceLabel Label describing import source.
 */
void ImportController::queueFilesForImport(
    const QStringList& filePaths,
    int athleteId,
    bool showResultDialog,
    const QString& sourceLabel)
{
    // TODO: Extract queue management logic from MainWindow
    emit filesQueued(filePaths.size(), sourceLabel);
}

/**
 * @brief Returns whether import operations can proceed.
 * @return True if database and athlete are ready for import.
 */
bool ImportController::canImportActivities() const
{
    if (!m_dbManager || !m_controller)
        return false;
    
    return m_dbManager->isOpen() && m_controller->currentAthleteId() > 0;
}

/**
 * @brief Updates import availability status based on current conditions.
 */
void ImportController::updateImportAvailability()
{
    const bool available = canImportActivities();
    emit importAvailabilityChanged(available);
    // TODO: Extract availability update logic from MainWindow
}

/**
 * @brief Gets the import progress model.
 * @return Pointer to ImportProgressModel, or nullptr if not set.
 */
ImportProgressModel* ImportController::progressModel() const
{
    return m_progressModel;
}

/**
 * @brief Gets the import queue.
 * @return Pointer to ImportQueue, or nullptr if not set.
 */
ImportQueue* ImportController::importQueue() const
{
    return m_importQueue;
}

/**
 * @brief Helper to finalize import batch summaries.
 */
void ImportController::finalizeBatches()
{
    // TODO: Extract batch finalization logic from MainWindow
    emit importBatchesCompleted();
}

/**
 * @brief Helper to handle activity import completion.
 */
void ImportController::onActivityImportComplete(int activityId)
{
    emit activityImported(activityId);
}
