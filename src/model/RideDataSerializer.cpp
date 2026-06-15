#include "RideDataSerializer.h"
#include "database/DatabaseManager.h"

#include <QDateTime>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariantList>

// ── Save ─────────────────────────────────────────────────────────────────────

int RideDataSerializer::saveRideToDatabase(
    const RideData&          rideData,
    const WorkoutStatistics& stats,
    int                      athleteId,
    const QString&           filePath,
    const QString&           fitHash,
    double                   normalizedPower,
    DatabaseManager&         dbManager,
    ActivityRepository&      activityRepo,
    ImportRepository&        importRepo,
    QString*                 errorOut)
{
    QSqlDatabase db = dbManager.database();

    // ── Duplicate check ───────────────────────────────────────────────────
    if (!fitHash.isEmpty() && activityRepo.hasFitHash(fitHash))
    {
        ImportRecord rec;
        rec.fileName = QFileInfo(filePath).fileName();
        rec.status   = "duplicate";
        importRepo.logImport(rec);

        if (errorOut)
            *errorOut = "Already imported (duplicate file).";
        return -1;
    }

    // ── Build Activity record ─────────────────────────────────────────────
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    Activity act;
    act.athleteId       = athleteId;
    act.fitHash         = fitHash;
    act.fileName        = QFileInfo(filePath).fileName();
    act.sport           = "Cycling";
    act.startTime       = rideData.activityStartTimeUtc.isEmpty() ? now : rideData.activityStartTimeUtc;
    act.endTime         = rideData.activityEndTimeUtc.isEmpty() ? act.startTime : rideData.activityEndTimeUtc;
    act.durationSec     = stats.durationSeconds;
    act.distanceM       = stats.totalDistanceKm * 1000.0;
    act.avgPower        = stats.averagePower;
    act.maxPower        = stats.maximumPower;
    act.normalizedPower = normalizedPower;
    act.avgHr           = stats.averageHeartRate;
    act.maxHr           = stats.maximumHeartRate;
    act.avgCadence      = stats.averageCadence;
    act.avgSpeed        = stats.averageSpeed;
    act.elevationGain   = stats.elevationGainM;
    act.importedAt      = now;

    // ── Transaction: activity row + all samples ───────────────────────────
    if (!db.transaction())
    {
        if (errorOut) *errorOut = db.lastError().text();
        return -1;
    }

    const int activityId = activityRepo.insertActivity(act);
    if (activityId < 0)
    {
        db.rollback();
        ImportRecord rec;
        rec.fileName     = act.fileName;
        rec.status       = "error";
        rec.errorMessage = "Failed to insert activity row.";
        importRepo.logImport(rec);
        if (errorOut) *errorOut = rec.errorMessage;
        return -1;
    }

    // Bulk-insert samples via execBatch
    QSqlQuery q(db);
    q.prepare(
        "INSERT INTO activity_samples("
        "  activity_id, elapsed_seconds,"
        "  power_total, heart_rate, cadence, speed,"
        "  latitude, longitude, altitude,"
        "  has_gps, has_power, has_heart_rate, has_cadence, has_speed, has_altitude"
        ") VALUES("
        "  :aid, :es,"
        "  :pw, :hr, :cad, :spd,"
        "  :lat, :lon, :alt,"
        "  :hgps, :hpw, :hhr, :hcad, :hspd, :halt"
        ")");

    QVariantList aids, es_, pw_, hr_, cad_, spd_;
    QVariantList lat_, lon_, alt_;
    QVariantList hgps, hpw, hhr, hcad, hspd, halt;

    for (const auto& r : rideData.records)
    {
        aids  << activityId;
        es_   << r.elapsedSeconds;
        pw_   << (r.hasPower     ? QVariant(r.power)     : QVariant());
        hr_   << (r.hasHeartRate ? QVariant(r.heartRate) : QVariant());
        cad_  << (r.hasCadence   ? QVariant(r.cadence)   : QVariant());
        spd_  << (r.hasSpeed     ? QVariant(r.speed)     : QVariant());
        lat_  << (r.hasGps       ? QVariant(r.latitude)  : QVariant());
        lon_  << (r.hasGps       ? QVariant(r.longitude) : QVariant());
        alt_  << (r.hasAltitude  ? QVariant(r.altitude)  : QVariant());
        hgps  << (r.hasGps       ? 1 : 0);
        hpw   << (r.hasPower     ? 1 : 0);
        hhr   << (r.hasHeartRate ? 1 : 0);
        hcad  << (r.hasCadence   ? 1 : 0);
        hspd  << (r.hasSpeed     ? 1 : 0);
        halt  << (r.hasAltitude  ? 1 : 0);
    }

    q.bindValue(":aid",  aids);
    q.bindValue(":es",   es_);
    q.bindValue(":pw",   pw_);
    q.bindValue(":hr",   hr_);
    q.bindValue(":cad",  cad_);
    q.bindValue(":spd",  spd_);
    q.bindValue(":lat",  lat_);
    q.bindValue(":lon",  lon_);
    q.bindValue(":alt",  alt_);
    q.bindValue(":hgps", hgps);
    q.bindValue(":hpw",  hpw);
    q.bindValue(":hhr",  hhr);
    q.bindValue(":hcad", hcad);
    q.bindValue(":hspd", hspd);
    q.bindValue(":halt", halt);

    if (!q.execBatch())
    {
        db.rollback();
        ImportRecord rec;
        rec.fileName     = act.fileName;
        rec.status       = "error";
        rec.errorMessage = "Failed to insert samples: " + q.lastError().text();
        importRepo.logImport(rec);
        if (errorOut) *errorOut = rec.errorMessage;
        return -1;
    }

    if (!db.commit())
    {
        db.rollback();
        if (errorOut) *errorOut = db.lastError().text();
        return -1;
    }

    // Log success
    ImportRecord rec;
    rec.fileName   = act.fileName;
    rec.status     = "success";
    rec.activityId = activityId;
    importRepo.logImport(rec);

    return activityId;
}

// ── Load ─────────────────────────────────────────────────────────────────────

RideData RideDataSerializer::loadRideFromDatabase(int activityId,
                                                   DatabaseManager& dbManager)
{
    RideData rideData;
    QSqlDatabase db = dbManager.database();

    // Pre-reserve vector to avoid repeated reallocations during large sample loads
    {
        QSqlQuery countQ(db);
        countQ.prepare("SELECT COUNT(*) FROM activity_samples WHERE activity_id=:id");
        countQ.bindValue(":id", activityId);
        if (countQ.exec() && countQ.next())
        {
            const int sampleCount = countQ.value(0).toInt();
            if (sampleCount > 0)
                rideData.records.reserve(sampleCount);
        }
    }

    QSqlQuery q(db);
    q.prepare(
        "SELECT elapsed_seconds,"
        "  power_total, heart_rate, cadence, speed,"
        "  latitude, longitude, altitude,"
        "  has_gps, has_power, has_heart_rate, has_cadence, has_speed, has_altitude"
        " FROM activity_samples"
        " WHERE activity_id=:id"
        " ORDER BY elapsed_seconds ASC");
    q.bindValue(":id", activityId);
    q.setForwardOnly(true);
    q.exec();

    while (q.next())
    {
        RideRecord r;
        int col = 0;
        r.elapsedSeconds = q.value(col++).toDouble();
        r.power          = q.value(col++).toDouble();
        r.heartRate      = q.value(col++).toDouble();
        r.cadence        = q.value(col++).toDouble();
        r.speed          = q.value(col++).toDouble();
        r.latitude       = q.value(col++).toDouble();
        r.longitude      = q.value(col++).toDouble();
        r.altitude       = q.value(col++).toDouble();
        r.hasGps         = q.value(col++).toBool();
        r.hasPower       = q.value(col++).toBool();
        r.hasHeartRate   = q.value(col++).toBool();
        r.hasCadence     = q.value(col++).toBool();
        r.hasSpeed       = q.value(col++).toBool();
        r.hasAltitude    = q.value(col++).toBool();
        rideData.records.push_back(r);
    }

    return rideData;
}
