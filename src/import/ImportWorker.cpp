// SPDX-License-Identifier: GPL-3

#include "ImportWorker.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include "analysis/TrainingLoad.h"
#include "analysis/WorkoutAnalyzer.h"
#include "database/ActivityRepository.h"
#include "database/DatabaseConnectionGuard.h"
#include "database/ImportRepository.h"
#include "fit/FitParser.h"

namespace
{
void applySqlitePragmas(QSqlDatabase& db)
{
    QSqlQuery q(db);
    q.exec("PRAGMA journal_mode=WAL");
    q.exec("PRAGMA synchronous=NORMAL");
    q.exec("PRAGMA temp_store=MEMORY");
    q.exec("PRAGMA foreign_keys=ON");
    q.exec("PRAGMA cache_size=-20000");
}
}

ImportWorker::ImportWorker(QObject* parent)
    : QObject(parent)
{}

void ImportWorker::setDatabasePath(const QString& path)
{
    m_dbPath = path;
}

void ImportWorker::processJob(const ImportJob& job)
{
    if (m_dbPath.isEmpty())
    {
        emit failed(job.jobId, job.batchId, job.filePath,
                    QStringLiteral("error"), QStringLiteral("No database path set."));
        return;
    }

    const int athleteId = job.athleteId.toInt();
    if (athleteId <= 0)
    {
        emit failed(job.jobId, job.batchId, job.filePath,
                    QStringLiteral("error"), QStringLiteral("No athlete selected."));
        return;
    }

    const QString connName = QStringLiteral("ImportWorker_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    try
    {
        DatabaseConnectionGuard guard(connName);
        QSqlDatabase db = guard.database();
        db.setDatabaseName(m_dbPath);
        if (!db.open())
        {
            emit failed(job.jobId, job.batchId, job.filePath,
                        QStringLiteral("error"), db.lastError().text());
            return;
        }

        applySqlitePragmas(db);
        emit progressChanged(job.jobId, job.batchId, job.filePath, 5.0, QStringLiteral("Opening file"));

        QString fitHash;
        QFile file(job.filePath);
        if (file.open(QIODevice::ReadOnly))
        {
            fitHash = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256).toHex();
            file.close();
        }

        ActivityRepository activityRepo(db);
        ImportRepository importRepo(db);

        const auto emitLoggedFailure = [&](const QString& reason, const QString& message)
        {
            ImportRecord rec;
            rec.fileName = QFileInfo(job.filePath).fileName();
            rec.status = reason == QStringLiteral("duplicate") || reason == QStringLiteral("time_overlap")
                ? QStringLiteral("duplicate")
                : QStringLiteral("error");
            rec.errorMessage = message;
            importRepo.logImport(rec);

            emit failed(job.jobId, job.batchId, job.filePath, reason, message);
        };

        if (!fitHash.isEmpty() && activityRepo.hasFitHash(fitHash))
        {
            emitLoggedFailure(QStringLiteral("duplicate"),
                              QStringLiteral("Already imported (duplicate file)."));
            return;
        }

        emit progressChanged(job.jobId, job.batchId, job.filePath, 15.0, QStringLiteral("Parsing FIT"));

        FitParser parser;
        RideData ride = parser.load(job.filePath);

        if (!ride.activityStartTimeUtc.isEmpty())
        {
            QSqlQuery overlapQ(db);
            overlapQ.prepare(
                "SELECT id FROM activities "
                "WHERE athlete_id=:aid AND activity_start_time=:st LIMIT 1");
            overlapQ.bindValue(":aid", athleteId);
            overlapQ.bindValue(":st", ride.activityStartTimeUtc);
            if (overlapQ.exec() && overlapQ.next())
            {
                emitLoggedFailure(QStringLiteral("time_overlap"),
                                  QStringLiteral("Potential duplicate activity (matching start time)."));
                return;
            }
        }

        emit progressChanged(job.jobId, job.batchId, job.filePath, 72.0, QStringLiteral("Storing activity"));

        WorkoutAnalyzer analyzer;
        const WorkoutStatistics stats = analyzer.analyze(ride);
        const double np = TrainingLoad::normalizedPower(ride);

        const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        Activity act;
        act.athleteId = athleteId;
        act.fitHash = fitHash;
        act.fileName = QFileInfo(job.filePath).fileName();
        act.sport = QStringLiteral("Cycling");
        act.startTime = ride.activityStartTimeUtc.isEmpty() ? now : ride.activityStartTimeUtc;
        act.endTime = ride.activityEndTimeUtc.isEmpty() ? act.startTime : ride.activityEndTimeUtc;
        act.durationSec = stats.durationSeconds;
        act.distanceM = stats.totalDistanceKm * 1000.0;
        act.avgPower = stats.averagePower;
        act.maxPower = stats.maximumPower;
        act.normalizedPower = np;
        act.avgHr = stats.averageHeartRate;
        act.maxHr = stats.maximumHeartRate;
        act.avgCadence = stats.averageCadence;
        act.avgSpeed = stats.averageSpeed;
        act.elevationGain = stats.elevationGainM;
        act.importedAt = now;

        if (!db.transaction())
        {
            emitLoggedFailure(QStringLiteral("error"), db.lastError().text());
            return;
        }

        const int activityId = activityRepo.insertActivity(act);
        if (activityId <= 0)
        {
            db.rollback();
            emitLoggedFailure(QStringLiteral("error"), QStringLiteral("Failed to insert activity."));
            return;
        }

        QCryptographicHash fingerprintHasher(QCryptographicHash::Sha256);
        fingerprintHasher.addData(QStringLiteral("Cycling").toUtf8());
        fingerprintHasher.addData(ride.activityStartTimeUtc.toUtf8());
        fingerprintHasher.addData(QString::number(static_cast<int>(stats.durationSeconds)).toUtf8());
        fingerprintHasher.addData(QString::number(static_cast<int>(stats.totalDistanceKm * 1000.0)).toUtf8());
        activityRepo.updateFingerprint(activityId, fingerprintHasher.result().toHex());

        QSqlQuery sampleQ(db);
        sampleQ.prepare(
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

        for (const RideRecord& r : ride.records)
        {
            aids << activityId;
            es_ << r.elapsedSeconds;
            pw_ << (r.hasPower ? QVariant(r.power) : QVariant());
            hr_ << (r.hasHeartRate ? QVariant(r.heartRate) : QVariant());
            cad_ << (r.hasCadence ? QVariant(r.cadence) : QVariant());
            spd_ << (r.hasSpeed ? QVariant(r.speed) : QVariant());
            lat_ << (r.hasGps ? QVariant(r.latitude) : QVariant());
            lon_ << (r.hasGps ? QVariant(r.longitude) : QVariant());
            alt_ << (r.hasAltitude ? QVariant(r.altitude) : QVariant());
            hgps << (r.hasGps ? 1 : 0);
            hpw << (r.hasPower ? 1 : 0);
            hhr << (r.hasHeartRate ? 1 : 0);
            hcad << (r.hasCadence ? 1 : 0);
            hspd << (r.hasSpeed ? 1 : 0);
            halt << (r.hasAltitude ? 1 : 0);
        }

        sampleQ.bindValue(":aid", aids);
        sampleQ.bindValue(":es", es_);
        sampleQ.bindValue(":pw", pw_);
        sampleQ.bindValue(":hr", hr_);
        sampleQ.bindValue(":cad", cad_);
        sampleQ.bindValue(":spd", spd_);
        sampleQ.bindValue(":lat", lat_);
        sampleQ.bindValue(":lon", lon_);
        sampleQ.bindValue(":alt", alt_);
        sampleQ.bindValue(":hgps", hgps);
        sampleQ.bindValue(":hpw", hpw);
        sampleQ.bindValue(":hhr", hhr);
        sampleQ.bindValue(":hcad", hcad);
        sampleQ.bindValue(":hspd", hspd);
        sampleQ.bindValue(":halt", halt);

        if (!sampleQ.execBatch())
        {
            db.rollback();
            emitLoggedFailure(QStringLiteral("error"),
                              QStringLiteral("Failed to insert samples: %1").arg(sampleQ.lastError().text()));
            return;
        }

        // Store optional import source metadata.
        if (!job.importSource.isEmpty())
            activityRepo.setImportSource(activityId, job.importSource);

        emit progressChanged(job.jobId, job.batchId, job.filePath, 96.0, QStringLiteral("Committing"));

        if (!db.commit())
        {
            db.rollback();
            emitLoggedFailure(QStringLiteral("error"), db.lastError().text());
            return;
        }

        ImportRecord rec;
        rec.fileName = act.fileName;
        rec.status = QStringLiteral("success");
        rec.activityId = activityId;
        importRepo.logImport(rec);

        emit progressChanged(job.jobId, job.batchId, job.filePath, 100.0, QStringLiteral("Done"));
        emit completed(job.jobId, job.batchId, job.filePath, activityId);
    }
    catch (const std::exception& ex)
    {
        emit failed(job.jobId, job.batchId, job.filePath,
                    QStringLiteral("error"), QString::fromLocal8Bit(ex.what()));
    }
    catch (...)
    {
        emit failed(job.jobId, job.batchId, job.filePath,
                    QStringLiteral("error"), QStringLiteral("Unknown import error."));
    }
}
