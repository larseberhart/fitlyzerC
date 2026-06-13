#include "IntervalRepository.h"

#include <QSqlDatabase>
#include <QSqlQuery>

IntervalRepository::IntervalRepository(QSqlDatabase& db)
    : m_db(db)
{}

int IntervalRepository::insertInterval(const IntervalRecord& interval)
{
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO intervals("
        "activity_id, start_sample, end_sample, name, type, avg_power, np, avg_hr, avg_cadence, notes"
        ") VALUES("
        ":activity_id, :start_sample, :end_sample, :name, :type, :avg_power, :np, :avg_hr, :avg_cadence, :notes"
        ")");

    q.bindValue(":activity_id", interval.activityId);
    q.bindValue(":start_sample", interval.startSample);
    q.bindValue(":end_sample", interval.endSample);
    q.bindValue(":name", interval.name);
    q.bindValue(":type", interval.type);
    q.bindValue(":avg_power", interval.avgPower);
    q.bindValue(":np", interval.np);
    q.bindValue(":avg_hr", interval.avgHr);
    q.bindValue(":avg_cadence", interval.avgCadence);
    q.bindValue(":notes", interval.notes);

    if (!q.exec())
        return -1;

    return q.lastInsertId().toInt();
}

bool IntervalRepository::deleteInterval(int intervalId)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM intervals WHERE id=:id");
    q.bindValue(":id", intervalId);
    return q.exec();
}

bool IntervalRepository::deleteIntervalsForActivity(int activityId)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM intervals WHERE activity_id=:id");
    q.bindValue(":id", activityId);
    return q.exec();
}

QList<IntervalRecord> IntervalRepository::listIntervalsForActivity(int activityId)
{
    QList<IntervalRecord> out;

    QSqlQuery q(m_db);
    q.prepare(
        "SELECT id, activity_id, start_sample, end_sample, name, type, avg_power, np, avg_hr, avg_cadence, notes "
        "FROM intervals WHERE activity_id=:id ORDER BY start_sample ASC");
    q.bindValue(":id", activityId);
    q.exec();

    while (q.next())
    {
        IntervalRecord record;
        int col = 0;
        record.id = q.value(col++).toInt();
        record.activityId = q.value(col++).toInt();
        record.startSample = q.value(col++).toInt();
        record.endSample = q.value(col++).toInt();
        record.name = q.value(col++).toString();
        record.type = q.value(col++).toString();
        record.avgPower = q.value(col++).toDouble();
        record.np = q.value(col++).toDouble();
        record.avgHr = q.value(col++).toDouble();
        record.avgCadence = q.value(col++).toDouble();
        record.notes = q.value(col++).toString();
        out.push_back(record);
    }

    return out;
}
