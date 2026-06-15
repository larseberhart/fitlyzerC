// SPDX-License-Identifier: GPL-3

/**
 * @file ClimbRepository.cpp
 * @brief Climb database persistence.
 *
 * Implements CRUD operations and queries for climb
 * records stored in the SQLite database.
 *
 * @author Lars EBERHART
 */

#include "ClimbRepository.h"
#include "SqlQueryHelper.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>

namespace
{
const char* kSelectCols =
    "id, activity_id, COALESCE(uuid,''), start_seconds, end_seconds,"
    " COALESCE(original_start_seconds,-1.0), COALESCE(original_end_seconds,-1.0),"
    " COALESCE(name,''), COALESCE(elevation_gain_m,0.0), COALESCE(length_km,0.0),"
    " COALESCE(average_gradient,0.0),"
    " COALESCE(avg_power,0.0), COALESCE(np,0.0), COALESCE(avg_hr,0.0),"
    " COALESCE(avg_cadence,0.0), COALESCE(vam,0.0),"
    " COALESCE(source,'auto'), COALESCE(locked,0), COALESCE(deleted,0),"
    " COALESCE(algorithm_version,1),"
    " COALESCE(notes,''), COALESCE(favorite,0), COALESCE(rating,0)";

ClimbRecord recordFromQuery(QSqlQuery& q)
{
    ClimbRecord r;
    int col = 0;
    r.id                  = q.value(col++).toInt();
    r.activityId          = q.value(col++).toInt();
    r.uuid                = q.value(col++).toString();
    r.startSeconds        = q.value(col++).toDouble();
    r.endSeconds          = q.value(col++).toDouble();
    r.originalStartSeconds = q.value(col++).toDouble();
    r.originalEndSeconds  = q.value(col++).toDouble();
    r.name                = q.value(col++).toString();
    r.elevationGainM      = q.value(col++).toDouble();
    r.lengthKm            = q.value(col++).toDouble();
    r.averageGradient     = q.value(col++).toDouble();
    r.avgPower            = q.value(col++).toDouble();
    r.np                  = q.value(col++).toDouble();
    r.avgHr               = q.value(col++).toDouble();
    r.avgCadence          = q.value(col++).toDouble();
    r.vam                 = q.value(col++).toDouble();
    r.source              = q.value(col++).toString();
    r.locked              = q.value(col++).toInt() != 0;
    r.deleted             = q.value(col++).toInt() != 0;
    r.algorithmVersion    = q.value(col++).toInt();
    r.notes               = q.value(col++).toString();
    r.favorite            = q.value(col++).toInt() != 0;
    r.rating              = q.value(col++).toInt();
    return r;
}
} // namespace

ClimbRepository::ClimbRepository(QSqlDatabase& db)
    : m_db(db)
{}

int ClimbRepository::insertClimb(const ClimbRecord& climb)
{
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO climbs("
        "  activity_id, uuid, start_seconds, end_seconds,"
        "  original_start_seconds, original_end_seconds,"
        "  name, elevation_gain_m, length_km, average_gradient,"
        "  avg_power, np, avg_hr, avg_cadence, vam,"
        "  source, locked, deleted, algorithm_version,"
        "  notes, favorite, rating"
        ") VALUES("
        "  :activity_id, :uuid, :start_seconds, :end_seconds,"
        "  :orig_start, :orig_end,"
        "  :name, :elevation_gain_m, :length_km, :average_gradient,"
        "  :avg_power, :np, :avg_hr, :avg_cadence, :vam,"
        "  :source, :locked, :deleted, :algorithm_version,"
        "  :notes, :favorite, :rating"
        ")");

    q.bindValue(":activity_id", climb.activityId);
    q.bindValue(":uuid", climb.uuid.isEmpty() ? QVariant() : QVariant(climb.uuid));
    q.bindValue(":start_seconds", climb.startSeconds);
    q.bindValue(":end_seconds", climb.endSeconds);
    q.bindValue(":orig_start", climb.originalStartSeconds < 0 ? QVariant() : QVariant(climb.originalStartSeconds));
    q.bindValue(":orig_end", climb.originalEndSeconds < 0 ? QVariant() : QVariant(climb.originalEndSeconds));
    q.bindValue(":name", climb.name);
    q.bindValue(":elevation_gain_m", climb.elevationGainM);
    q.bindValue(":length_km", climb.lengthKm);
    q.bindValue(":average_gradient", climb.averageGradient);
    q.bindValue(":avg_power", climb.avgPower);
    q.bindValue(":np", climb.np);
    q.bindValue(":avg_hr", climb.avgHr);
    q.bindValue(":avg_cadence", climb.avgCadence);
    q.bindValue(":vam", climb.vam);
    q.bindValue(":source", climb.source);
    SqlQueryHelper::bindBoolValue(q, ":locked", climb.locked);
    SqlQueryHelper::bindBoolValue(q, ":deleted", climb.deleted);
    q.bindValue(":algorithm_version", climb.algorithmVersion);
    q.bindValue(":notes", climb.notes);
    q.bindValue(":favorite", climb.favorite ? 1 : 0);
    q.bindValue(":rating", climb.rating);

    if (!q.exec())
        return -1;
    return q.lastInsertId().toInt();
}

bool ClimbRepository::updateClimb(const ClimbRecord& climb)
{
    QSqlQuery q(m_db);
    q.prepare(
        "UPDATE climbs SET"
        "  start_seconds    = :start_seconds,"
        "  end_seconds      = :end_seconds,"
        "  name             = :name,"
        "  elevation_gain_m = :elevation_gain_m,"
        "  length_km        = :length_km,"
        "  average_gradient = :average_gradient,"
        "  avg_power        = :avg_power,"
        "  np               = :np,"
        "  avg_hr           = :avg_hr,"
        "  avg_cadence      = :avg_cadence,"
        "  vam              = :vam,"
        "  source           = :source,"
        "  locked           = :locked,"
        "  deleted          = :deleted,"
        "  notes            = :notes,"
        "  favorite         = :favorite,"
        "  rating           = :rating,"
        "  updated_at       = CURRENT_TIMESTAMP"
        " WHERE id = :id");

    q.bindValue(":start_seconds", climb.startSeconds);
    q.bindValue(":end_seconds", climb.endSeconds);
    q.bindValue(":name", climb.name);
    q.bindValue(":elevation_gain_m", climb.elevationGainM);
    q.bindValue(":length_km", climb.lengthKm);
    q.bindValue(":average_gradient", climb.averageGradient);
    q.bindValue(":avg_power", climb.avgPower);
    q.bindValue(":np", climb.np);
    q.bindValue(":avg_hr", climb.avgHr);
    q.bindValue(":avg_cadence", climb.avgCadence);
    q.bindValue(":vam", climb.vam);
    q.bindValue(":source", climb.source);
    SqlQueryHelper::bindBoolValue(q, ":locked", climb.locked);
    SqlQueryHelper::bindBoolValue(q, ":deleted", climb.deleted);
    q.bindValue(":notes", climb.notes);
    q.bindValue(":favorite", climb.favorite ? 1 : 0);
    q.bindValue(":rating", climb.rating);
    q.bindValue(":id", climb.id);
    return q.exec();
}

bool ClimbRepository::softDeleteClimb(int climbId)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE climbs SET deleted=1, locked=1, updated_at=CURRENT_TIMESTAMP WHERE id=:id");
    q.bindValue(":id", climbId);
    return q.exec();
}

bool ClimbRepository::hardDeleteClimb(int climbId)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM climbs WHERE id=:id");
    q.bindValue(":id", climbId);
    return q.exec();
}

bool ClimbRepository::deleteClimbsForActivity(int activityId)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM climbs WHERE activity_id=:id");
    q.bindValue(":id", activityId);
    return q.exec();
}

ClimbRecord ClimbRepository::getClimb(int climbId)
{
    QSqlQuery q(m_db);
    q.prepare(QString("SELECT %1 FROM climbs WHERE id=:id").arg(kSelectCols));
    q.bindValue(":id", climbId);
    if (q.exec() && q.next())
        return recordFromQuery(q);
    return {};
}

QList<ClimbRecord> ClimbRepository::climbsForActivity(int activityId)
{
    QList<ClimbRecord> out;
    QSqlQuery q(m_db);
    q.prepare(QString("SELECT %1 FROM climbs WHERE activity_id=:id AND COALESCE(deleted,0)=0"
                      " ORDER BY start_seconds ASC").arg(kSelectCols));
    q.bindValue(":id", activityId);
    if (!q.exec())
        return out;
    while (q.next())
        out.push_back(recordFromQuery(q));
    return out;
}

QList<ClimbRecord> ClimbRepository::deletedClimbsForActivity(int activityId)
{
    QList<ClimbRecord> out;
    QSqlQuery q(m_db);
    q.prepare(QString("SELECT %1 FROM climbs WHERE activity_id=:id AND COALESCE(deleted,0)=1"
                      " ORDER BY start_seconds ASC").arg(kSelectCols));
    q.bindValue(":id", activityId);
    if (!q.exec())
        return out;
    while (q.next())
        out.push_back(recordFromQuery(q));
    return out;
}

bool ClimbRepository::hasClimbs(int activityId)
{
    QSqlQuery q(m_db);
    q.prepare(SqlQueryHelper::selectExistsNonDeleted("climbs"));
    q.bindValue(":id", activityId);
    return q.exec() && q.next();
}

bool ClimbRepository::hasClimbsEver(int activityId)
{
    QSqlQuery q(m_db);
    q.prepare(SqlQueryHelper::selectExistsEver("climbs"));
    q.bindValue(":id", activityId);
    return q.exec() && q.next();
}

bool ClimbRepository::hasLockedClimbs(int activityId)
{
    QSqlQuery q(m_db);
    q.prepare(SqlQueryHelper::selectExistsLocked("climbs"));
    q.bindValue(":id", activityId);
    return q.exec() && q.next();
}