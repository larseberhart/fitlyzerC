// SPDX-License-Identifier: GPL-3


#include "AnalysisQueue.h"
#include "AnalysisWorker.h"

#include <QThread>

AnalysisQueue::AnalysisQueue(QObject* parent)
    : QObject(parent)
{
    m_thread = new QThread(this);
    m_worker = new AnalysisWorker;
    m_worker->moveToThread(m_thread);

    connect(m_worker, &AnalysisWorker::taskCompleted,
            this,     &AnalysisQueue::onTaskCompleted);
    connect(m_worker, &AnalysisWorker::taskFailed,
            this,     &AnalysisQueue::onTaskFailed);

    m_thread->start();
}

AnalysisQueue::~AnalysisQueue()
{
    m_thread->quit();
    m_thread->wait(3000);
    delete m_worker;
}

void AnalysisQueue::setDatabasePath(const QString& path)
{
    m_dbPath = path;
    // Forward to worker on its own thread.
    QMetaObject::invokeMethod(m_worker, [this, path]() {
        m_worker->setDatabasePath(path);
    }, Qt::QueuedConnection);
}

void AnalysisQueue::enqueue(int activityId)
{
    m_queue.enqueue(activityId);
    emit pendingCountChanged(m_queue.size());
    if (!m_busy)
        processNext();
}

int AnalysisQueue::pendingCount() const
{
    return m_queue.size() + (m_busy ? 1 : 0);
}

bool AnalysisQueue::isBusy() const
{
    return m_busy;
}

void AnalysisQueue::processNext()
{
    if (m_queue.isEmpty() || m_busy)
        return;

    m_busy = true;
    const int activityId = m_queue.dequeue();
    emit pendingCountChanged(m_queue.size());

    QMetaObject::invokeMethod(m_worker, [this, activityId]() {
        m_worker->processTask(activityId);
    }, Qt::QueuedConnection);
}

void AnalysisQueue::onTaskCompleted(int activityId)
{
    m_busy = false;
    emit taskCompleted(activityId);
    processNext();
}

void AnalysisQueue::onTaskFailed(int activityId, const QString& error)
{
    m_busy = false;
    emit taskFailed(activityId, error);
    processNext();
}