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
        "  activity_id, start_sample, end_sample, name, type,"
        "  avg_power, np, avg_hr, avg_cadence, notes,"
        "  source, locked, deleted, algorithm_version, uuid"
        ") VALUES("
        "  :activity_id, :start_sample, :end_sample, :name, :type,"
        "  :avg_power, :np, :avg_hr, :avg_cadence, :notes,"
        "  :source, :locked, :deleted, :algorithm_version, :uuid"
        ")");

    q.bindValue(":activity_id",       interval.activityId);
    q.bindValue(":start_sample",      interval.startSample);
    q.bindValue(":end_sample",        interval.endSample);
    q.bindValue(":name",              interval.name);
    q.bindValue(":type",              interval.type);
    q.bindValue(":avg_power",         interval.avgPower);
    q.bindValue(":np",                interval.np);
    q.bindValue(":avg_hr",            interval.avgHr);
    q.bindValue(":avg_cadence",       interval.avgCadence);
    q.bindValue(":notes",             interval.notes);
    q.bindValue(":source",            interval.source);
    q.bindValue(":locked",            interval.locked   ? 1 : 0);
    q.bindValue(":deleted",           interval.deleted  ? 1 : 0);
    q.bindValue(":algorithm_version", interval.algorithmVersion);
    q.bindValue(":uuid",              interval.uuid.isEmpty() ? QVariant() : QVariant(interval.uuid));

    if (!q.exec()) return -1;
    return q.lastInsertId().toInt();
}

bool IntervalRepository::updateInterval(const IntervalRecord& interval)
{
    QSqlQuery q(m_db);
    q.prepare(
        "UPDATE intervals SET"
        "  start_sample = :start_sample,"
        "  end_sample   = :end_sample,"
        "  name         = :name,"
        "  type         = :type,"
        "  avg_power    = :avg_power,"
        "  np           = :np,"
        "  avg_hr       = :avg_hr,"
        "  avg_cadence  = :avg_cadence,"
        "  notes        = :notes,"
        "  source       = :source,"
        "  locked       = :locked,"
        "  deleted      = :deleted,"
        "  updated_at   = CURRENT_TIMESTAMP"
        " WHERE id = :id");

    q.bindValue(":start_sample", interval.startSample);
    q.bindValue(":end_sample",   interval.endSample);
    q.bindValue(":name",         interval.name);
    q.bindValue(":type",         interval.type);
    q.bindValue(":avg_power",    interval.avgPower);
    q.bindValue(":np",           interval.np);
    q.bindValue(":avg_hr",       interval.avgHr);
    q.bindValue(":avg_cadence",  interval.avgCadence);
    q.bindValue(":notes",        interval.notes);
    q.bindValue(":source",       interval.source);
    q.bindValue(":locked",       interval.locked  ? 1 : 0);
    q.bindValue(":deleted",      interval.deleted ? 1 : 0);
    q.bindValue(":id",           interval.id);
    return q.exec();
}

bool IntervalRepository::softDeleteInterval(int intervalId)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE intervals SET deleted=1, locked=1, updated_at=CURRENT_TIMESTAMP WHERE id=:id");
    q.bindValue(":id", intervalId);
    return q.exec();
}

bool IntervalRepository::deleteInterval(int intervalId)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM intervals WHERE id=:id");
    q.bindValue(":id", intervalId);
    return q.exec();
// deleteInterval remains a hard delete (used from UI when permanent removal is wanted)
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
        "SELECT id, activity_id, start_sample, end_sample, name, type,"
        " avg_power, np, avg_hr, avg_cadence, notes,"
        " COALESCE(source,'auto'), COALESCE(locked,0), COALESCE(deleted,0),"
        " COALESCE(algorithm_version,1), COALESCE(uuid,'')"
        " FROM intervals WHERE activity_id=:id AND COALESCE(deleted,0)=0"
        " ORDER BY start_sample ASC");
    q.bindValue(":id", activityId);
    q.exec();

    while (q.next())
    {
        IntervalRecord record;
        int col = 0;
        record.id              = q.value(col++).toInt();
        record.activityId      = q.value(col++).toInt();
        record.startSample     = q.value(col++).toInt();
        record.endSample       = q.value(col++).toInt();
        record.name            = q.value(col++).toString();
        record.type            = q.value(col++).toString();
        record.avgPower        = q.value(col++).toDouble();
        record.np              = q.value(col++).toDouble();
        record.avgHr           = q.value(col++).toDouble();
        record.avgCadence      = q.value(col++).toDouble();
        record.notes           = q.value(col++).toString();
        record.source          = q.value(col++).toString();
        record.locked          = q.value(col++).toInt() != 0;
        record.deleted         = q.value(col++).toInt() != 0;
        record.algorithmVersion = q.value(col++).toInt();
        record.uuid            = q.value(col++).toString();
        out.push_back(record);
    }
    return out;
}

bool IntervalRepository::hasLockedIntervals(int activityId)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT 1 FROM intervals WHERE activity_id=:id AND locked=1 LIMIT 1");
    q.bindValue(":id", activityId);
    return q.exec() && q.next();
}
