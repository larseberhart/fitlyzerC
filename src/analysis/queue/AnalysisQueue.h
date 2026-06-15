#pragma once

#include <QObject>
#include <QQueue>
#include <QString>

class QThread;
class AnalysisWorker;

/// Manages a sequential queue of background analysis tasks.
/// Each task processes one activity (detect climbs + intervals, store results).
class AnalysisQueue : public QObject
{
    Q_OBJECT
public:
    explicit AnalysisQueue(QObject* parent = nullptr);
    ~AnalysisQueue() override;

    void setDatabasePath(const QString& path);

    /// Queue an activity for background analysis.
    void enqueue(int activityId);

    int  pendingCount() const;
    bool isBusy()       const;

signals:
    void taskCompleted(int activityId);
    void taskFailed(int activityId, const QString& error);
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
