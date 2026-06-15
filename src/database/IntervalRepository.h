#pragma once

#include <QList>
#include <QString>

class QSqlDatabase;

struct IntervalRecord
{
    int id = -1;
    int activityId = -1;
    int startSample = 0;
    int endSample = 0;
    QString name;
    QString type;
    double avgPower = 0.0;
    double np = 0.0;
    double avgHr = 0.0;
    double avgCadence = 0.0;
    QString notes;
    QString source           = QStringLiteral("auto"); // "auto", "manual", "edited"
    bool    locked           = false;
    bool    deleted          = false;
    int     algorithmVersion = 1;
    QString uuid;
};

class IntervalRepository
{
public:
    explicit IntervalRepository(QSqlDatabase& db);

    int  insertInterval(const IntervalRecord& interval);
    bool updateInterval(const IntervalRecord& interval);

    /// Soft-delete: marks deleted=1.
    bool softDeleteInterval(int intervalId);
    /// Hard-delete (for full reset before re-detect).
    bool deleteInterval(int intervalId);
    bool deleteIntervalsForActivity(int activityId);

    /// Returns non-deleted intervals only.
    QList<IntervalRecord> listIntervalsForActivity(int activityId);
    bool hasLockedIntervals(int activityId);

private:
    QSqlDatabase& m_db;
};
