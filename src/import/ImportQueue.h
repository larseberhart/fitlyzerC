// SPDX-License-Identifier: GPL-3

#pragma once

#include <QObject>
#include <QQueue>
#include <QString>

#include "ImportJob.h"

class ImportWorker;
class QThread;

/**
 * @brief Sequential queue that dispatches background FIT import jobs.
 *
 * Jobs are processed one-at-a-time on a worker thread. The queue emits
 * lifecycle signals for UI updates and batch bookkeeping.
 */
class ImportQueue : public QObject
{
    Q_OBJECT
public:
    explicit ImportQueue(QObject* parent = nullptr);
    ~ImportQueue() override;

    /// @brief Sets the SQLite database path used by the worker.
    void setDatabasePath(const QString& path);

    /// @brief Enqueues a file import request.
    /// @param filePath Absolute file path to a FIT file.
    /// @param athleteId Target athlete id as string.
    /// @param batchId Caller-defined batch id for grouped summaries.
    /// @param importSource Source metadata saved with the imported activity.
    void enqueue(const QString& filePath,
                 const QString& athleteId,
                 const QString& batchId,
                 const QString& importSource);

    /// @brief Returns active jobs count (queued + currently running).
    int pendingCount() const;

    /// @brief Returns true when a job is currently running.
    bool isBusy() const;

signals:
    /// @brief Emitted when a job enters the queue.
    void jobQueued(const QString& jobId,
                   const QString& batchId,
                   const QString& filePath,
                   int activeCount);

    /// @brief Emitted when a queued job begins processing.
    void jobStarted(const QString& jobId,
                    const QString& batchId,
                    const QString& filePath,
                    int activeCount);

    /// @brief Emitted with progress updates from the running job.
    void jobProgress(const QString& jobId,
                     const QString& batchId,
                     const QString& filePath,
                     double percentage,
                     const QString& statusText,
                     int activeCount);

    /// @brief Emitted when a job successfully creates an activity.
    void jobFinished(const QString& jobId,
                     const QString& batchId,
                     const QString& filePath,
                     int activityId,
                     int activeCount);

    /// @brief Emitted when a job fails or is rejected as duplicate.
    ///
    /// `reason` is a stable category such as `duplicate`, `time_overlap`, or
    /// `error`; `errorMessage` provides user-facing details.
    void jobFailed(const QString& jobId,
                   const QString& batchId,
                   const QString& filePath,
                   const QString& reason,
                   const QString& errorMessage,
                   int activeCount);

    /// @brief Emitted whenever running/pending counts change.
    void queueStateChanged(int runningCount, int pendingCount);

    /// @brief Emitted after the last queued/running job is fully processed.
    void queueIdle();

private slots:
    void onWorkerProgress(const QString& jobId,
                          const QString& batchId,
                          const QString& filePath,
                          double percentage,
                          const QString& statusText);

    void onWorkerCompleted(const QString& jobId,
                           const QString& batchId,
                           const QString& filePath,
                           int activityId);

    void onWorkerFailed(const QString& jobId,
                        const QString& batchId,
                        const QString& filePath,
                        const QString& reason,
                        const QString& errorMessage);

private:
    void processNext();
    int activeCountUnsafe() const;

    QThread* m_thread = nullptr;
    ImportWorker* m_worker = nullptr;
    QQueue<ImportJob> m_queue;
    bool m_busy = false;
    QString m_dbPath;
    ImportJob m_runningJob;
};
