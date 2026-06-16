// SPDX-License-Identifier: GPL-3

#pragma once

#include <QObject>
#include <QString>

#include "ImportJob.h"

/**
 * @brief Worker that imports FIT files in a dedicated background thread.
 *
 * The worker parses FIT payloads, performs duplicate checks, writes activity
 * and sample rows, and emits progress/final status signals.
 */
class ImportWorker : public QObject
{
    Q_OBJECT
public:
    explicit ImportWorker(QObject* parent = nullptr);

    /// @brief Sets the SQLite database path used for imports.
    void setDatabasePath(const QString& path);

public slots:
    /// @brief Processes one queued import job.
    void processJob(const ImportJob& job);

signals:
    /// @brief Progress update for the currently running import job.
    void progressChanged(const QString& jobId,
                        const QString& batchId,
                        const QString& filePath,
                        double percentage,
                        const QString& statusText);

    /// @brief Emitted when an activity was imported successfully.
    void completed(const QString& jobId,
                   const QString& batchId,
                   const QString& filePath,
                   int activityId);

    /// @brief Emitted when import fails or is rejected as duplicate.
    ///
    /// `reason` is a machine-readable category (`duplicate`, `time_overlap`,
    /// `error`) and `errorMessage` contains details.
    void failed(const QString& jobId,
                const QString& batchId,
                const QString& filePath,
                const QString& reason,
                const QString& errorMessage);

private:
    QString m_dbPath;
};
