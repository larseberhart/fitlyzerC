#include "ClimbRepository.h"

#include <QSqlDatabase>
#include <QSqlQuery>

ClimbRepository::ClimbRepository(QSqlDatabase& db)
    : m_db(db)
{}

int ClimbRepository::insertClimb(const ClimbRecord& climb)
{
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO climbs("
        "  activity_id, start_seconds, end_seconds, name,"
        "  elevation_gain_m, length_km, average_gradient, source, locked"
        ") VALUES("
        "  :activity_id, :start_seconds, :end_seconds, :name,"
        "  :elevation_gain_m, :length_km, :average_gradient, :source, :locked"
        ")");

    q.bindValue(":activity_id",       climb.activityId);
    q.bindValue(":start_seconds",     climb.startSeconds);
    q.bindValue(":end_seconds",       climb.endSeconds);
    q.bindValue(":name",              climb.name);
    q.bindValue(":elevation_gain_m",  climb.elevationGainM);
    q.bindValue(":length_km",         climb.lengthKm);
    q.bindValue(":average_gradient",  climb.averageGradient);
    q.bindValue(":source",            climb.source);
    q.bindValue(":locked",            climb.locked ? 1 : 0);

    if (!q.exec())
        return -1;

    return q.lastInsertId().toInt();
}

bool ClimbRepository::updateClimb(const ClimbRecord& climb)
{
    QSqlQuery q(m_db);
    q.prepare(
        "UPDATE climbs SET"
        "  start_seconds = :start_seconds,"
        "  end_seconds   = :end_seconds,"
        "  name          = :name,"
        "  elevation_gain_m = :elevation_gain_m,"
        "  length_km     = :length_km,"
        "  average_gradient = :average_gradient,"
        "  source        = :source,"
        "  locked        = :locked,"
        "  updated_at    = CURRENT_TIMESTAMP"
        " WHERE id = :id");

    q.bindValue(":start_seconds",     climb.startSeconds);
    q.bindValue(":end_seconds",       climb.endSeconds);
    q.bindValue(":name",              climb.name);
    q.bindValue(":elevation_gain_m",  climb.elevationGainM);
    q.bindValue(":length_km",         climb.lengthKm);
    q.bindValue(":average_gradient",  climb.averageGradient);
    q.bindValue(":source",            climb.source);
    q.bindValue(":locked",            climb.locked ? 1 : 0);
    q.bindValue(":id",                climb.id);

    return q.exec();
}

bool ClimbRepository::deleteClimb(int climbId)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM climbs WHERE id = :id");
    q.bindValue(":id", climbId);
    return q.exec();
}

bool ClimbRepository::deleteClimbsForActivity(int activityId)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM climbs WHERE activity_id = :id");
    q.bindValue(":id", activityId);
    return q.exec();
}

QList<ClimbRecord> ClimbRepository::climbsForActivity(int activityId)
{
    QList<ClimbRecord> out;
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT id, activity_id, start_seconds, end_seconds, name,"
        "  elevation_gain_m, length_km, average_gradient, source, locked"
        " FROM climbs WHERE activity_id = :id ORDER BY start_seconds ASC");
    q.bindValue(":id", activityId);
    if (!q.exec())
        return out;

    while (q.next())
    {
        ClimbRecord r;
        int col = 0;
        r.id               = q.value(col++).toInt();
        r.activityId       = q.value(col++).toInt();
        r.startSeconds     = q.value(col++).toDouble();
        r.endSeconds       = q.value(col++).toDouble();
        r.name             = q.value(col++).toString();
        r.elevationGainM   = q.value(col++).toDouble();
        r.lengthKm         = q.value(col++).toDouble();
        r.averageGradient  = q.value(col++).toDouble();
        r.source           = q.value(col++).toString();
        r.locked           = q.value(col++).toInt() != 0;
        out.push_back(r);
    }
    return out;
}

bool ClimbRepository::hasClimbs(int activityId)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT 1 FROM climbs WHERE activity_id = :id LIMIT 1");
    q.bindValue(":id", activityId);
    return q.exec() && q.next();
}

bool ClimbRepository::hasLockedClimbs(int activityId)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT 1 FROM climbs WHERE activity_id = :id AND locked = 1 LIMIT 1");
    q.bindValue(":id", activityId);
    return q.exec() && q.next();
}
