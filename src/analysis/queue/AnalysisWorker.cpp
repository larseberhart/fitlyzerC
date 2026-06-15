#include "AnalysisWorker.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
#include <QUuid>

#include "analysis/ClimbDetector.h"
#include "analysis/ClimbMetrics.h"
#include "analysis/IntervalDetector.h"
#include "core/AnalysisVersions.h"
#include "core/ActivityAnalysisFlags.h"
#include "database/ActivityRepository.h"
#include "database/ClimbRepository.h"
#include "database/IntervalRepository.h"
#include "model/RideDataSerializer.h"
#include "utils/UuidGenerator.h"

AnalysisWorker::AnalysisWorker(QObject* parent)
    : QObject(parent)
{}

void AnalysisWorker::setDatabasePath(const QString& path)
{
    m_dbPath = path;
}

void AnalysisWorker::processTask(int activityId)
{
    if (m_dbPath.isEmpty())
    {
        emit taskFailed(activityId, QStringLiteral("No database path set."));
        return;
    }

    // Each worker thread gets its own SQLite connection.
    const QString connName = QStringLiteral("AnalysisWorker_%1_%2")
        .arg(activityId)
        .arg(reinterpret_cast<quintptr>(QThread::currentThread()));

    auto cleanUp = [&connName]() { QSqlDatabase::removeDatabase(connName); };

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(m_dbPath);

    if (!db.open())
    {
        cleanUp();
        emit taskFailed(activityId, db.lastError().text());
        return;
    }

    // ── Load ride data ──────────────────────────────────────────────────
    // We need a temporary DatabaseManager-like wrapper.  The simplest
    // approach: use RideDataSerializer directly with a dummy DatabaseManager
    // created around the already-open db connection.
    // Since RideDataSerializer::loadRideFromDatabase takes a DatabaseManager&,
    // and we can't construct one from an existing connection, we load samples
    // directly here.

    QSqlQuery samplesQuery(db);
    samplesQuery.prepare(
        "SELECT elapsed_seconds, power_total, heart_rate, cadence, speed,"
        "  latitude, longitude, altitude,"
        "  has_gps, has_power, has_heart_rate, has_cadence, has_speed, has_altitude"
        " FROM activity_samples WHERE activity_id=:id ORDER BY elapsed_seconds ASC");
    samplesQuery.bindValue(":id", activityId);

    RideData ride;
    if (samplesQuery.exec())
    {
        while (samplesQuery.next())
        {
            RideRecord rec;
            rec.elapsedSeconds = samplesQuery.value(0).toDouble();
            rec.power          = samplesQuery.value(1).toDouble();
            rec.heartRate      = samplesQuery.value(2).toDouble();
            rec.cadence        = samplesQuery.value(3).toDouble();
            rec.speed          = samplesQuery.value(4).toDouble();
            rec.latitude       = samplesQuery.value(5).toDouble();
            rec.longitude      = samplesQuery.value(6).toDouble();
            rec.altitude       = samplesQuery.value(7).toDouble();
            rec.hasGps         = samplesQuery.value(8).toInt() != 0;
            rec.hasPower       = samplesQuery.value(9).toInt() != 0;
            rec.hasHeartRate   = samplesQuery.value(10).toInt() != 0;
            rec.hasCadence     = samplesQuery.value(11).toInt() != 0;
            rec.hasSpeed       = samplesQuery.value(12).toInt() != 0;
            rec.hasAltitude    = samplesQuery.value(13).toInt() != 0;
            ride.records.push_back(rec);
        }
    }

    if (ride.records.empty())
    {
        db.close();
        cleanUp();
        emit taskFailed(activityId, QStringLiteral("Activity has no samples."));
        return;
    }

    ClimbRepository    climbRepo(db);
    IntervalRepository intervalRepo(db);

    // ── Skip if already analyzed ─────────────────────────────────────────
    {
        QSqlQuery flagQ(db);
        flagQ.prepare("SELECT COALESCE(analysis_flags,0) FROM activities WHERE id=:id");
        flagQ.bindValue(":id", activityId);
        if (flagQ.exec() && flagQ.next())
        {
            const int flags = flagQ.value(0).toInt();
            const bool hasClimbs    = (flags & ActivityAnalysisFlags::HasClimbs)    != 0;
            const bool hasIntervals = (flags & ActivityAnalysisFlags::HasIntervals) != 0;
            if (hasClimbs && hasIntervals)
            {
                db.close();
                cleanUp();
                emit taskCompleted(activityId);
                return;
            }
        }
    }

    // ── Detect and store climbs ──────────────────────────────────────────
    if (!climbRepo.hasClimbsEver(activityId))
    {
        ClimbDetector::Config climbCfg;

        // Load per-activity settings if available
        {
            QSqlQuery settQ(db);
            settQ.prepare(
                "SELECT climb_min_gain, climb_min_length, climb_min_gradient"
                " FROM activity_analysis_settings WHERE activity_id=:id");
            settQ.bindValue(":id", activityId);
            if (settQ.exec() && settQ.next())
            {
                if (!settQ.value(0).isNull()) climbCfg.minElevationGainM  = settQ.value(0).toDouble();
                if (!settQ.value(1).isNull()) climbCfg.minLengthKm        = settQ.value(1).toDouble();
                if (!settQ.value(2).isNull()) climbCfg.minAverageGradient = settQ.value(2).toDouble();
            }
        }

        const std::vector<Climb> climbs = ClimbDetector::detect(ride, climbCfg);

        // Load deleted climbs for resurrection prevention
        const QList<ClimbRecord> deleted = climbRepo.deletedClimbsForActivity(activityId);

        const std::vector<double> cumulative = ClimbDetector::buildCumulativeDistanceMeters(ride);
        const std::vector<double> smoothed   = ClimbDetector::smoothAltitudeByDistance(
            ride, cumulative, climbCfg.elevationSmoothingMeters);
        const std::vector<double> gradient   = ClimbDetector::computeLocalGradientPct(
            smoothed, cumulative, climbCfg.gradientWindowHalfMeters);

        int ordinal = 1;
        for (const Climb& c : climbs)
        {
            // Resurrection prevention
            bool overlapsDeleted = false;
            for (const ClimbRecord& d : deleted)
            {
                if (c.startSeconds < d.endSeconds && c.endSeconds > d.startSeconds)
                {
                    overlapsDeleted = true;
                    break;
                }
            }
            if (overlapsDeleted) continue;

            ClimbRecord record;
            record.activityId             = activityId;
            record.uuid                   = UuidGenerator::create();
            record.startSeconds           = c.startSeconds;
            record.endSeconds             = c.endSeconds;
            record.originalStartSeconds   = c.startSeconds;
            record.originalEndSeconds     = c.endSeconds;
            record.name                   = c.name;
            record.elevationGainM         = c.elevationGainM;
            record.lengthKm               = c.lengthKm;
            record.averageGradient        = c.averageGradient;
            record.avgPower               = c.averagePower;
            record.np                     = c.normalizedPower;
            record.avgHr                  = c.averageHeartRate;
            record.avgCadence             = c.averageCadence;
            record.vam                    = c.vam;
            record.source                 = QStringLiteral("auto");
            record.locked                 = false;
            record.deleted                = false;
            record.algorithmVersion       = AnalysisVersions::ClimbDetection;
            climbRepo.insertClimb(record);
            ++ordinal;
        }

        // Mark HasClimbs flag
        QSqlQuery upFlags(db);
        upFlags.prepare(
            "UPDATE activities SET analysis_flags = COALESCE(analysis_flags,0) | :f WHERE id=:id");
        upFlags.bindValue(":f", ActivityAnalysisFlags::HasClimbs);
        upFlags.bindValue(":id", activityId);
        upFlags.exec();
    }

    // ── Detect and store intervals ───────────────────────────────────────
    {
        const QList<IntervalRecord> existing = intervalRepo.listIntervalsForActivity(activityId);
        if (existing.isEmpty())
        {
            // Resolve FTP for this activity's date
            double ftp = 250.0;
            {
                QSqlQuery ftpQ(db);
                ftpQ.prepare(
                    "SELECT ah.ftp_watts FROM ftp_history ah"
                    " JOIN activities a ON a.athlete_id = ah.athlete_id"
                    " WHERE a.id=:id AND ah.effective_from <= SUBSTR(a.activity_start_time,1,10)"
                    " ORDER BY ah.effective_from DESC LIMIT 1");
                ftpQ.bindValue(":id", activityId);
                if (ftpQ.exec() && ftpQ.next())
                    ftp = ftpQ.value(0).toDouble();
            }

            IntervalDetector::Config ivCfg;
            ivCfg.ftp = ftp;
            const std::vector<Interval> intervals = IntervalDetector::detect(ride, ivCfg);

            for (const Interval& iv : intervals)
            {
                IntervalRecord record;
                record.activityId       = activityId;
                record.uuid             = UuidGenerator::create();
                record.startSample      = static_cast<int>(std::round(iv.startSeconds));
                record.endSample        = static_cast<int>(std::round(iv.endSeconds));
                record.name             = QString::fromStdString(iv.label);
                record.type             = QString::fromStdString(iv.label);
                record.avgPower         = iv.averagePower;
                record.np               = iv.normalizedPower;
                record.avgHr            = iv.averageHeartRate;
                record.avgCadence       = iv.averageCadence;
                record.source           = QStringLiteral("auto");
                record.locked           = false;
                record.deleted          = false;
                record.algorithmVersion = AnalysisVersions::IntervalDetection;
                intervalRepo.insertInterval(record);
            }

            QSqlQuery upFlags(db);
            upFlags.prepare(
                "UPDATE activities SET analysis_flags = COALESCE(analysis_flags,0) | :f WHERE id=:id");
            upFlags.bindValue(":f", ActivityAnalysisFlags::HasIntervals);
            upFlags.bindValue(":id", activityId);
            upFlags.exec();
        }
    }

    db.close();
    cleanUp();
    emit taskCompleted(activityId);
}
