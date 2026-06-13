#include "ActivityRepository.h"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>

ActivityRepository::ActivityRepository(QSqlDatabase& db)
    : m_db(db)
{}

// ── Helpers ───────────────────────────────────────────────────────────────────

static Activity activityFromQuery(QSqlQuery& q)
{
    Activity a;
    int col = 0;
    a.id              = q.value(col++).toInt();
    a.athleteId       = q.value(col++).toInt();
    a.fitHash         = q.value(col++).toString();
    a.fileName        = q.value(col++).toString();
    a.sport           = q.value(col++).toString();
    a.startTime       = q.value(col++).toString();
    a.endTime         = q.value(col++).toString();
    a.durationSec     = q.value(col++).toDouble();
    a.distanceM       = q.value(col++).toDouble();
    a.avgPower        = q.value(col++).toDouble();
    a.maxPower        = q.value(col++).toDouble();
    a.normalizedPower = q.value(col++).toDouble();
    a.avgHr           = q.value(col++).toDouble();
    a.maxHr           = q.value(col++).toDouble();
    a.avgCadence      = q.value(col++).toDouble();
    a.avgSpeed        = q.value(col++).toDouble();
    a.elevationGain   = q.value(col++).toDouble();
    a.notes           = q.value(col++).toString();
    a.weatherNotes    = q.value(col++).toString();
    a.temperature     = q.value(col++).toDouble();
    a.weather         = q.value(col++).toString();
    a.wind            = q.value(col++).toString();
    a.rpe             = q.value(col++).toInt();
    a.fatigue         = q.value(col++).toInt();
    a.sleepHours      = q.value(col++).toDouble();
    a.weightKg        = q.value(col++).toDouble();
    a.bike            = q.value(col++).toString();
    a.equipment       = q.value(col++).toString();
    a.importedAt      = q.value(col++).toString();
    return a;
}

static const char* kSelectCols =
    "id, athlete_id, fit_hash, file_name, sport, start_time, end_time,"
    " duration_sec, distance_m, avg_power, max_power, normalized_power,"
    " avg_hr, max_hr, avg_cadence, avg_speed, elevation_gain,"
    " notes, weather_notes, temperature, weather, wind, rpe, fatigue, sleep,"
    " weight, bike, equipment, imported_at";

// ── Public API ────────────────────────────────────────────────────────────────

int ActivityRepository::insertActivity(const Activity& a)
{
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO activities("
        "  athlete_id, fit_hash, file_name, sport, start_time, end_time,"
        "  duration_sec, distance_m, avg_power, max_power, normalized_power,"
        "  avg_hr, max_hr, avg_cadence, avg_speed, elevation_gain,"
        "  notes, weather_notes, temperature, weather, wind, rpe, fatigue, sleep,"
        "  weight, bike, equipment, imported_at"
        ") VALUES("
        "  :aid, :fh, :fn, :sport, :st, :et,"
        "  :dur, :dist, :ap, :mp, :np,"
        "  :ah, :mh, :ac, :as_, :eg,"
        "  :notes, :wn, :temp, :weather, :wind, :rpe, :fatigue, :sleep,"
        "  :weight, :bike, :equipment, :ts"
        ")");

    q.bindValue(":aid",   a.athleteId);
    q.bindValue(":fh",    a.fitHash.isEmpty()   ? QVariant() : QVariant(a.fitHash));
    q.bindValue(":fn",    a.fileName);
    q.bindValue(":sport", a.sport.isEmpty()     ? QVariant() : QVariant(a.sport));
    q.bindValue(":st",    a.startTime.isEmpty() ? QVariant() : QVariant(a.startTime));
    q.bindValue(":et",    a.endTime.isEmpty()   ? QVariant() : QVariant(a.endTime));
    q.bindValue(":dur",   a.durationSec);
    q.bindValue(":dist",  a.distanceM);
    q.bindValue(":ap",    a.avgPower);
    q.bindValue(":mp",    a.maxPower);
    q.bindValue(":np",    a.normalizedPower);
    q.bindValue(":ah",    a.avgHr);
    q.bindValue(":mh",    a.maxHr);
    q.bindValue(":ac",    a.avgCadence);
    q.bindValue(":as_",   a.avgSpeed);
    q.bindValue(":eg",    a.elevationGain);
    q.bindValue(":notes", a.notes);
    q.bindValue(":wn",    a.weatherNotes);
    q.bindValue(":temp",  a.temperature);
    q.bindValue(":weather", a.weather.isEmpty() ? QVariant() : QVariant(a.weather));
    q.bindValue(":wind", a.wind.isEmpty() ? QVariant() : QVariant(a.wind));
    q.bindValue(":rpe", a.rpe > 0 ? QVariant(a.rpe) : QVariant());
    q.bindValue(":fatigue", a.fatigue > 0 ? QVariant(a.fatigue) : QVariant());
    q.bindValue(":sleep", a.sleepHours > 0.0 ? QVariant(a.sleepHours) : QVariant());
    q.bindValue(":weight", a.weightKg > 0.0 ? QVariant(a.weightKg) : QVariant());
    q.bindValue(":bike", a.bike.isEmpty() ? QVariant() : QVariant(a.bike));
    q.bindValue(":equipment", a.equipment.isEmpty() ? QVariant() : QVariant(a.equipment));
    q.bindValue(":ts",    a.importedAt.isEmpty()
                          ? QDateTime::currentDateTimeUtc().toString(Qt::ISODate)
                          : a.importedAt);

    if (!q.exec()) return -1;
    return q.lastInsertId().toInt();
}

Activity ActivityRepository::getActivity(int activityId)
{
    QSqlQuery q(m_db);
    q.prepare(QString("SELECT %1 FROM activities WHERE id=:id").arg(kSelectCols));
    q.bindValue(":id", activityId);
    q.exec();
    if (q.next()) return activityFromQuery(q);
    return {};
}

QList<Activity> ActivityRepository::listActivities(int athleteId)
{
    QList<Activity> result;
    QSqlQuery q(m_db);
    if (athleteId > 0)
    {
        q.prepare(QString("SELECT %1 FROM activities WHERE athlete_id=:aid"
                          " ORDER BY imported_at DESC").arg(kSelectCols));
        q.bindValue(":aid", athleteId);
    }
    else
    {
        q.prepare(QString("SELECT %1 FROM activities ORDER BY imported_at DESC")
                  .arg(kSelectCols));
    }
    q.exec();
    while (q.next())
        result.append(activityFromQuery(q));
    return result;
}

bool ActivityRepository::deleteActivity(int activityId)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM activities WHERE id=:id");
    q.bindValue(":id", activityId);
    return q.exec();
}

bool ActivityRepository::hasFitHash(const QString& hash)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT 1 FROM activities WHERE fit_hash=:h LIMIT 1");
    q.bindValue(":h", hash);
    q.exec();
    return q.next();
}

bool ActivityRepository::updateNotes(int activityId,
                                     const QString& notes,
                                     const QString& weatherNotes)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE activities SET notes=:n, weather_notes=:wn WHERE id=:id");
    q.bindValue(":n",   notes);
    q.bindValue(":wn",  weatherNotes);
    q.bindValue(":id",  activityId);
    return q.exec();
}

bool ActivityRepository::updateActivityProperties(int activityId, const Activity& a)
{
    QSqlQuery q(m_db);
    q.prepare(
        "UPDATE activities SET "
        "notes=:notes, weather_notes=:wn, temperature=:temp, weather=:weather, wind=:wind,"
        "rpe=:rpe, fatigue=:fatigue, sleep=:sleep, weight=:weight, bike=:bike, equipment=:equipment "
        "WHERE id=:id");
    q.bindValue(":notes", a.notes);
    q.bindValue(":wn", a.weatherNotes);
    q.bindValue(":temp", a.temperature > 0.0 ? QVariant(a.temperature) : QVariant());
    q.bindValue(":weather", a.weather.isEmpty() ? QVariant() : QVariant(a.weather));
    q.bindValue(":wind", a.wind.isEmpty() ? QVariant() : QVariant(a.wind));
    q.bindValue(":rpe", a.rpe > 0 ? QVariant(a.rpe) : QVariant());
    q.bindValue(":fatigue", a.fatigue > 0 ? QVariant(a.fatigue) : QVariant());
    q.bindValue(":sleep", a.sleepHours > 0.0 ? QVariant(a.sleepHours) : QVariant());
    q.bindValue(":weight", a.weightKg > 0.0 ? QVariant(a.weightKg) : QVariant());
    q.bindValue(":bike", a.bike.isEmpty() ? QVariant() : QVariant(a.bike));
    q.bindValue(":equipment", a.equipment.isEmpty() ? QVariant() : QVariant(a.equipment));
    q.bindValue(":id", activityId);
    if (!q.exec())
        return false;

    // Keep equipment usage in sync for service interval tracking.
    QSqlQuery details(m_db);
    details.prepare("SELECT athlete_id, distance_m, duration_sec FROM activities WHERE id=:id");
    details.bindValue(":id", activityId);
    if (!details.exec() || !details.next())
        return true;

    const int athleteId = details.value(0).toInt();
    const double distanceKm = details.value(1).toDouble() / 1000.0;
    const double durationSec = details.value(2).toDouble();

    QSqlQuery clearUsage(m_db);
    clearUsage.prepare("DELETE FROM equipment_usage WHERE activity_id=:id");
    clearUsage.bindValue(":id", activityId);
    clearUsage.exec();

    QStringList equipmentNames;
    if (!a.bike.trimmed().isEmpty())
        equipmentNames << a.bike.trimmed();
    if (!a.equipment.trimmed().isEmpty())
    {
        const QStringList split = a.equipment.split(',', Qt::SkipEmptyParts);
        for (const QString& entry : split)
        {
            const QString trimmed = entry.trimmed();
            if (!trimmed.isEmpty())
                equipmentNames << trimmed;
        }
    }
    equipmentNames.removeDuplicates();

    for (const QString& name : equipmentNames)
    {
        int equipmentId = -1;
        QSqlQuery findEq(m_db);
        findEq.prepare("SELECT id FROM equipment WHERE athlete_id=:athlete_id AND name=:name LIMIT 1");
        findEq.bindValue(":athlete_id", athleteId);
        findEq.bindValue(":name", name);
        if (findEq.exec() && findEq.next())
        {
            equipmentId = findEq.value(0).toInt();
        }
        else
        {
            QSqlQuery insertEq(m_db);
            insertEq.prepare(
                "INSERT INTO equipment(athlete_id, name, category, notes) "
                "VALUES(:athlete_id, :name, :category, :notes)");
            insertEq.bindValue(":athlete_id", athleteId);
            insertEq.bindValue(":name", name);
            insertEq.bindValue(":category", name.compare(a.bike, Qt::CaseInsensitive) == 0 ? "Bike" : "Component");
            insertEq.bindValue(":notes", QString());
            if (insertEq.exec())
                equipmentId = insertEq.lastInsertId().toInt();
        }

        if (equipmentId <= 0)
            continue;

        QSqlQuery usage(m_db);
        usage.prepare(
            "INSERT INTO equipment_usage(equipment_id, activity_id, distance_km, duration_sec) "
            "VALUES(:equipment_id, :activity_id, :distance_km, :duration_sec)");
        usage.bindValue(":equipment_id", equipmentId);
        usage.bindValue(":activity_id", activityId);
        usage.bindValue(":distance_km", distanceKm);
        usage.bindValue(":duration_sec", durationSec);
        usage.exec();
    }

    return true;
}
