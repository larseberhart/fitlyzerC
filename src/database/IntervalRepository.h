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
};

class IntervalRepository
{
public:
    explicit IntervalRepository(QSqlDatabase& db);

    int insertInterval(const IntervalRecord& interval);
    bool deleteInterval(int intervalId);
    bool deleteIntervalsForActivity(int activityId);
    QList<IntervalRecord> listIntervalsForActivity(int activityId);

private:
    QSqlDatabase& m_db;
};
