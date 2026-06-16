// SPDX-License-Identifier: GPL-3

#include "ImportQueue.h"

#include <QThread>
#include <QUuid>

#include "ImportWorker.h"

ImportQueue::ImportQueue(QObject* parent)
    : QObject(parent)
{
    m_thread = new QThread(this);
    m_worker = new ImportWorker;
    m_worker->moveToThread(m_thread);

    connect(m_worker, &ImportWorker::progressChanged,
            this, &ImportQueue::onWorkerProgress);
    connect(m_worker, &ImportWorker::completed,
            this, &ImportQueue::onWorkerCompleted);
    connect(m_worker, &ImportWorker::failed,
            this, &ImportQueue::onWorkerFailed);

    m_thread->start();
}

ImportQueue::~ImportQueue()
{
    m_thread->quit();
    m_thread->wait(3000);
    delete m_worker;
}

void ImportQueue::setDatabasePath(const QString& path)
{
    m_dbPath = path;
    QMetaObject::invokeMethod(m_worker, [this, path]()
    {
        m_worker->setDatabasePath(path);
    }, Qt::QueuedConnection);
}

void ImportQueue::enqueue(const QString& filePath,
                          const QString& athleteId,
                          const QString& batchId,
                          const QString& importSource)
{
    ImportJob job;
    job.jobId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    job.batchId = batchId;
    job.filePath = filePath;
    job.athleteId = athleteId;
    job.importSource = importSource;
    job.state = ImportJob::State::Queued;
    job.progress = 0.0;

    m_queue.enqueue(job);

    emit jobQueued(job.jobId, job.batchId, job.filePath, activeCountUnsafe());
    emit queueStateChanged(m_busy ? 1 : 0, m_queue.size());

    if (!m_busy)
        processNext();
}

int ImportQueue::pendingCount() const
{
    return activeCountUnsafe();
}

bool ImportQueue::isBusy() const
{
    return m_busy;
}

void ImportQueue::onWorkerProgress(const QString& jobId,
                                   const QString& batchId,
                                   const QString& filePath,
                                   double percentage,
                                   const QString& statusText)
{
    emit jobProgress(jobId, batchId, filePath, percentage, statusText, activeCountUnsafe());
}

void ImportQueue::onWorkerCompleted(const QString& jobId,
                                    const QString& batchId,
                                    const QString& filePath,
                                    int activityId)
{
    m_busy = false;
    emit jobFinished(jobId, batchId, filePath, activityId, activeCountUnsafe());
    emit queueStateChanged(0, m_queue.size());

    if (!m_queue.isEmpty())
    {
        processNext();
        return;
    }

    emit queueIdle();
}

void ImportQueue::onWorkerFailed(const QString& jobId,
                                 const QString& batchId,
                                 const QString& filePath,
                                 const QString& reason,
                                 const QString& errorMessage)
{
    m_busy = false;
    emit jobFailed(jobId, batchId, filePath, reason, errorMessage, activeCountUnsafe());
    emit queueStateChanged(0, m_queue.size());

    if (!m_queue.isEmpty())
    {
        processNext();
        return;
    }

    emit queueIdle();
}

void ImportQueue::processNext()
{
    if (m_busy || m_queue.isEmpty())
        return;

    m_busy = true;
    m_runningJob = m_queue.dequeue();

    emit jobStarted(m_runningJob.jobId, m_runningJob.batchId, m_runningJob.filePath, activeCountUnsafe());
    emit queueStateChanged(1, m_queue.size());

    const ImportJob job = m_runningJob;
    QMetaObject::invokeMethod(m_worker, [this, job]()
    {
        m_worker->processJob(job);
    }, Qt::QueuedConnection);
}

int ImportQueue::activeCountUnsafe() const
{
    return m_queue.size() + (m_busy ? 1 : 0);
}
