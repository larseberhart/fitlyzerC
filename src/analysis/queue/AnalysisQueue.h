// SPDX-License-Identifier: GPL-3


#pragma once

#include <QObject>
#include <QQueue>
#include <QString>

class QThread;
class AnalysisWorker;

/**
 * @brief Manages sequential background analysis task queue.
 *
 * Processes activities one at a time in a background thread,
 * detecting climbs, intervals, and computing analysis metrics.
 */
class AnalysisQueue : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Constructs analysis queue.
     * @param parent Parent object.
     */
    explicit AnalysisQueue(QObject* parent = nullptr);
    ~AnalysisQueue() override;

    /**
     * @brief Sets the database path for analysis operations.
     * @param path Database file path.
     */
    void setDatabasePath(const QString& path);

    /**
     * @brief Queues an activity for background analysis.
     * @param activityId Activity ID to analyze.
     */
    void enqueue(int activityId);

    /**
     * @brief Gets the number of pending analysis tasks.
     * @return Count of queued activities.
     */
    int  pendingCount() const;

    /**
     * @brief Checks if analysis is currently running.
     * @return True if a task is being processed.
     */
    bool isBusy()       const;

signals:
    /// \signal Emitted when a task completes successfully.
    /// \param activityId Activity ID that was analyzed.
    void taskCompleted(int activityId);

    /// \signal Emitted when a task fails.
    /// \param activityId Activity ID that failed.
    /// \param error Error message describing the failure.
    void taskFailed(int activityId, const QString& error);

    /// \signal Emitted when pending task count changes.
    /// \param count Number of pending analysis tasks.
    void pendingCountChanged(int count);

private slots:
    void onTaskCompleted(int activityId);
    void onTaskFailed(int activityId, const QString& error);

private:
    void processNext();

    QThread*         m_thread  = nullptr;
    AnalysisWorker*  m_worker  = nullptr;
    QQueue<int>      m_queue;
    bool             m_busy    = false;
    QString          m_dbPath;
};