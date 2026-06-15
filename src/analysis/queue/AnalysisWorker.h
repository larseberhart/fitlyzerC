// SPDX-License-Identifier: GPL-3

/**
 * @file AnalysisWorker.h
 * @brief Analysis component for AnalysisWorker.
 *
 * Implements analysis logic used to compute cycling metrics, detect patterns, and derive activity insights.
 *
 * Responsibilities:
 * - Provide analysis-specific functionality for activity processing
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QObject>
#include <QString>

/**
 * @brief Worker thread for background activity analysis.
 *
 * Processes analysis tasks (detect climbs, intervals, compute metrics)
 * synchronously in a dedicated thread.
 */
class AnalysisWorker : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Constructs analysis worker.
     * @param parent Parent object.
     */
    explicit AnalysisWorker(QObject* parent = nullptr);

    /**
     * @brief Sets database path for analysis operations.
     * @param path Database file path.
     */
    void setDatabasePath(const QString& path);

public slots:
    /**
     * @brief Processes an activity analysis task.
     * @param activityId Activity ID to analyze.
     */
    void processTask(int activityId);

signals:
    /// \signal Emitted when a task completes successfully.
    /// \param activityId Activity ID that was analyzed.
    void taskCompleted(int activityId);

    /// \signal Emitted when a task fails.
    /// \param activityId Activity ID that failed.
    /// \param error Error message describing the failure.
    void taskFailed(int activityId, const QString& error);

private:
    QString m_dbPath;
};