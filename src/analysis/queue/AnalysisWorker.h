#pragma once

#include <QObject>
#include <QString>

/// Runs on a dedicated QThread.  Receives analysis tasks from AnalysisQueue
/// and processes each one synchronously (detect climbs + intervals, write to DB).
class AnalysisWorker : public QObject
{
    Q_OBJECT
public:
    explicit AnalysisWorker(QObject* parent = nullptr);

    void setDatabasePath(const QString& path);

public slots:
    void processTask(int activityId);

signals:
    void taskCompleted(int activityId);
    void taskFailed(int activityId, const QString& error);

private:
    QString m_dbPath;
};
