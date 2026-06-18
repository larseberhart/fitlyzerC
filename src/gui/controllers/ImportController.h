// SPDX-License-Identifier: GPL-3

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>

class WorkoutController;
class DatabaseManager;
class ImportQueue;
class ImportProgressModel;
class ImportStatusWidget;

/**
 * @brief Manages activity import operations and UI.
 *
 * Handles:
 * - Import queue management
 * - Import status widget
 * - Import progress tracking
 * - Import completion and results
 * - Import error handling
 * - File batch import coordination
 */
class ImportController : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the import controller.
     * @param controller Pointer to WorkoutController for data access.
     * @param dbManager Pointer to DatabaseManager for import destination.
     * @param parent Parent object.
     */
    explicit ImportController(
        WorkoutController* controller,
        DatabaseManager* dbManager,
        QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~ImportController() override;

    /**
     * @brief Sets the import queue.
     * @param importQueue Pointer to ImportQueue instance.
     */
    void setImportQueue(ImportQueue* importQueue);

    /**
     * @brief Sets the import progress model.
     * @param progressModel Pointer to ImportProgressModel instance.
     */
    void setProgressModel(ImportProgressModel* progressModel);

    /**
     * @brief Sets the import status widget.
     * @param statusWidget Pointer to ImportStatusWidget instance.
     */
    void setStatusWidget(ImportStatusWidget* statusWidget);

    /**
     * @brief Queues a batch of files for import.
     * @param filePaths List of FIT file paths to import.
     * @param athleteId ID of target athlete.
     * @param showResultDialog Whether to show import result dialog.
     * @param sourceLabel Label describing import source.
     */
    void queueFilesForImport(
        const QStringList& filePaths,
        int athleteId,
        bool showResultDialog,
        const QString& sourceLabel);

    /**
     * @brief Returns whether import operations can proceed.
     * @return True if database and athlete are ready for import.
     */
    bool canImportActivities() const;

    /**
     * @brief Updates import availability status based on current conditions.
     */
    void updateImportAvailability();

    /**
     * @brief Gets the import progress model.
     * @return Pointer to ImportProgressModel, or nullptr if not set.
     */
    ImportProgressModel* progressModel() const;

    /**
     * @brief Gets the import queue.
     * @return Pointer to ImportQueue, or nullptr if not set.
     */
    ImportQueue* importQueue() const;

signals:
    /// @brief Emitted when files are queued for import.
    void filesQueued(int count, const QString& source);

    /// @brief Emitted when an activity has been successfully imported.
    void activityImported(int activityId);

    /// @brief Emitted when import batches complete.
    void importBatchesCompleted();

    /// @brief Emitted when import availability changes.
    void importAvailabilityChanged(bool available);

    /// @brief Emitted when an error occurs during import.
    void errorOccurred(const QString& message);

private:
    /// @brief Helper to finalize import batch summaries.
    void finalizeBatches();

    /// @brief Helper to handle activity import completion.
    void onActivityImportComplete(int activityId);

    WorkoutController* m_controller;
    DatabaseManager* m_dbManager;
    ImportQueue* m_importQueue = nullptr;
    ImportProgressModel* m_progressModel = nullptr;
    ImportStatusWidget* m_statusWidget = nullptr;
};
