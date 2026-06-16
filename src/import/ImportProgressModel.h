// SPDX-License-Identifier: GPL-3

#pragma once

#include <QObject>
#include <QString>

class ImportQueue;

/**
 * @brief UI-facing aggregate state for background import queue activity.
 *
 * Tracks currently active jobs and latest progress text for status widgets.
 */
class ImportProgressModel : public QObject
{
    Q_OBJECT
public:
    explicit ImportProgressModel(QObject* parent = nullptr);

    /// @brief Connects queue signals and starts mirroring import state.
    void attachQueue(ImportQueue* queue);

    int activeJobs() const { return m_activeJobs; }
    int completedJobs() const { return m_completedJobs; }
    int failedJobs() const { return m_failedJobs; }
    double progress() const { return m_progress; }
    QString currentFileName() const { return m_currentFileName; }
    QString statusText() const { return m_statusText; }

signals:
    /// @brief Emitted whenever visible model state changes.
    void changed();

private:
    int m_activeJobs = 0;
    int m_completedJobs = 0;
    int m_failedJobs = 0;
    double m_progress = 0.0;
    QString m_currentFileName;
    QString m_statusText;
};
