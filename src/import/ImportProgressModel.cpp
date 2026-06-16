// SPDX-License-Identifier: GPL-3

#include "ImportProgressModel.h"

#include <QFileInfo>

#include "ImportQueue.h"

ImportProgressModel::ImportProgressModel(QObject* parent)
    : QObject(parent)
{}

void ImportProgressModel::attachQueue(ImportQueue* queue)
{
    if (!queue)
        return;

    connect(queue, &ImportQueue::jobStarted, this,
            [this](const QString&, const QString&, const QString& filePath, int activeCount)
    {
        m_activeJobs = activeCount;
        m_progress = 0.0;
        m_currentFileName = QFileInfo(filePath).fileName();
        m_statusText = QStringLiteral("Starting");
        emit changed();
    });

    connect(queue, &ImportQueue::jobProgress, this,
            [this](const QString&, const QString&, const QString& filePath,
                   double percentage, const QString& statusText, int activeCount)
    {
        m_activeJobs = activeCount;
        m_progress = percentage;
        m_currentFileName = QFileInfo(filePath).fileName();
        m_statusText = statusText;
        emit changed();
    });

    connect(queue, &ImportQueue::jobFinished, this,
            [this](const QString&, const QString&, const QString&, int, int activeCount)
    {
        m_activeJobs = activeCount;
        ++m_completedJobs;
        if (m_activeJobs <= 0)
        {
            m_progress = 100.0;
            m_statusText = QStringLiteral("Done");
        }
        emit changed();
    });

    connect(queue, &ImportQueue::jobFailed, this,
            [this](const QString&, const QString&, const QString&, const QString&, const QString&, int activeCount)
    {
        m_activeJobs = activeCount;
        ++m_failedJobs;
        if (m_activeJobs <= 0)
            m_statusText = QStringLiteral("Finished with errors");
        emit changed();
    });

    connect(queue, &ImportQueue::queueIdle, this, [this]()
    {
        m_activeJobs = 0;
        m_currentFileName.clear();
        m_statusText.clear();
        m_progress = 0.0;
        emit changed();
    });
}
