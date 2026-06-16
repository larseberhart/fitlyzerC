// SPDX-License-Identifier: GPL-3


#include "WorkoutController.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QSqlQuery>

#include <algorithm>
#include <cmath>

#include "core/ActivityAnalysisFlags.h"
#include "core/AnalysisVersions.h"
#include "database/ActivityAnalysisSettingsRepository.h"
#include "utils/UuidGenerator.h"
#include "analysis/ClimbDetector.h"
#include "analysis/ClimbMetrics.h"
#include "analysis/TrainingLoad.h"
#include "analysis/TrainingMetrics.h"
#include "core/zones/ZoneCalculator.h"
#include "database/ClimbRepository.h"
#include "database/DatabaseManager.h"
#include "database/IntervalRepository.h"
#include "fit/FitParser.h"
#include "model/RideDataSerializer.h"

namespace
{
QDate activityDateForFtp(const Activity& activity)
{
    const QString rawDate = !activity.startTime.isEmpty()
        ? activity.startTime.left(10)
        : activity.importedAt.left(10);
    return QDate::fromString(rawDate, Qt::ISODate);
}

int resolveAthleteFtpForDate(AthleteRepository& repo, int athleteId, const QDate& date)
{
    if (date.isValid())
    {
        const int historicalFtp = repo.getFtpForDate(athleteId, date);
        if (historicalFtp > 0)
            return historicalFtp;
    }

    const Athlete athlete = repo.getAthlete(athleteId);
    return athlete.ftpWatts > 0 ? athlete.ftpWatts : 250;
}
}

WorkoutController::WorkoutController(QObject* parent)
    : QObject(parent)
{}

void WorkoutController::setFTP(double ftp)
{
    m_ftp = ftp;
    if (!m_rideData.records.empty())
        reanalyze();
    emit ftpChanged(ftp);
}

void WorkoutController::setDatabaseManager(DatabaseManager* dbManager)
{
    m_dbManager = dbManager;
}

void WorkoutController::setCurrentAthlete(int athleteId)
{
    m_currentAthleteId = athleteId;
    if (m_dbManager && m_dbManager->isOpen() && athleteId > 0)
    {
        auto db = m_dbManager->database();
        QDate ftpDate = QDate::currentDate();
        if (m_currentActivityId > 0)
        {
            ActivityRepository actRepo(db);
            const Activity activity = actRepo.getActivity(m_currentActivityId);
            const QDate activityDate = activityDateForFtp(activity);
            if (activityDate.isValid())
                ftpDate = activityDate;
        }

        AthleteRepository repo(db);
        setFTP(static_cast<double>(resolveAthleteFtpForDate(repo, athleteId, ftpDate)));
    }
}

bool WorkoutController::loadFile(const QString& path, QString& errorOut)
{
    try
    {
        FitParser parser;
        m_rideData = parser.load(path);
        m_currentActivityId = -1;
        reanalyze();
        emit workoutLoaded();
        return true;
    }
    catch (const std::exception& ex)
    {
        errorOut = QString::fromLocal8Bit(ex.what());
        return false;
    }
}

int WorkoutController::importFile(const QString& path, QString& errorOut,
                                   bool allowTimeOverlap,
                                   bool runAnalysis,
                                   const QString& importSource,
                                   DuplicateActivityInfo* duplicateInfo,
                                   ImportResult* result)
{
    if (!m_dbManager || !m_dbManager->isOpen())
    {
        errorOut = "No database is open.";
        if (result) *result = ImportResult::Failed;
        return -1;
    }
    if (m_currentAthleteId < 0)
    {
        errorOut = "No athlete selected.";
        if (result) *result = ImportResult::Failed;
        return -1;
    }

    RideData ride;
    try
    {
        FitParser parser;
        ride = parser.load(path);
    }
    catch (const std::exception& ex)
    {
        errorOut = QString::fromLocal8Bit(ex.what());
        if (result) *result = ImportResult::Failed;
        return -1;
    }

    QFile f(path);
    QString fitHash;
    if (f.open(QIODevice::ReadOnly))
    {
        fitHash = QCryptographicHash::hash(f.readAll(), QCryptographicHash::Sha256)
                  .toHex();
        f.close();
    }

    // ── Level 2: Time-overlap detection ───────────────────────────────────
    if (!allowTimeOverlap && !ride.activityStartTimeUtc.isEmpty())
    {
        auto db = m_dbManager->database();
        QSqlQuery q(db);
        q.prepare(
            "SELECT id, activity_start_time, end_time, duration_sec "
            "FROM activities WHERE athlete_id=:aid AND activity_start_time=:st LIMIT 1");
        q.bindValue(":aid", m_currentAthleteId);
        q.bindValue(":st",  ride.activityStartTimeUtc);
        if (q.exec() && q.next())
        {
            if (duplicateInfo)
            {
                duplicateInfo->existingActivityId = q.value(0).toInt();
                duplicateInfo->existingStartUtc = q.value(1).toString();
                duplicateInfo->existingDurationSeconds = q.value(3).toDouble();
                duplicateInfo->newStartUtc = ride.activityStartTimeUtc;
                duplicateInfo->newDurationSeconds =
                    ride.records.empty() ? 0.0 : ride.records.back().elapsedSeconds;
            }

            // Return TimeOverlap result with descriptive error message
            errorOut = QString("A duplicate activity with the same start time already exists");
            if (result) *result = ImportResult::TimeOverlap;
            return -1;
        }
    }

    WorkoutAnalyzer analyzer;
    WorkoutStatistics stats = analyzer.analyze(ride);
    const double np = TrainingLoad::normalizedPower(ride);

    auto db = m_dbManager->database();
    ActivityRepository actRepo(db);
    ImportRepository   impRepo(db);

    const int activityId = RideDataSerializer::saveRideToDatabase(
        ride, stats, m_currentAthleteId, path, fitHash,
        np, *m_dbManager, actRepo, impRepo, &errorOut);

    if (activityId < 0)
    {
        if (result) *result = ImportResult::Failed;
        return -1;
    }

    // Store fingerprint (sport + start_time + rounded duration + rounded distance)
    {
        QCryptographicHash fpHasher(QCryptographicHash::Sha256);
        fpHasher.addData(QStringLiteral("Cycling").toUtf8());
        fpHasher.addData(ride.activityStartTimeUtc.toUtf8());
        fpHasher.addData(QString::number(static_cast<int>(stats.durationSeconds)).toUtf8());
        fpHasher.addData(QString::number(static_cast<int>(stats.totalDistanceKm * 1000.0)).toUtf8());
        const QString fingerprint = fpHasher.result().toHex();
        actRepo.updateFingerprint(activityId, fingerprint);
    }

    // Store import source
    if (!importSource.isEmpty())
        actRepo.setImportSource(activityId, importSource);

    emit activityImported(activityId);

    if (runAnalysis)
        loadActivity(activityId, errorOut);
    else if (m_analysisQueue)
        m_analysisQueue->enqueue(activityId);

    if (result) *result = ImportResult::Success;
    return activityId;
}

bool WorkoutController::loadActivity(int activityId, QString& errorOut)
{
    if (!m_dbManager || !m_dbManager->isOpen())
    {
        errorOut = "No database is open.";
        return false;
    }

    m_rideData = RideDataSerializer::loadRideFromDatabase(activityId, *m_dbManager);
    if (m_rideData.records.empty())
    {
        errorOut = "Activity has no samples in the database.";
        return false;
    }

    m_currentActivityId = activityId;

    if (m_currentAthleteId > 0)
    {
        auto db = m_dbManager->database();
        ActivityRepository actRepo(db);
        const Activity activity = actRepo.getActivity(activityId);
        const QDate ftpDate = activityDateForFtp(activity);

        AthleteRepository repo(db);
        m_ftp = resolveAthleteFtpForDate(
            repo,
            m_currentAthleteId,
            ftpDate.isValid() ? ftpDate : QDate::currentDate());
    }

    reanalyze();
    loadOrGenerateIntervals();
    loadOrGenerateClimbs();
    emit workoutLoaded();
    return true;
}

void WorkoutController::reanalyze()
{
    m_statistics = m_analyzer.analyze(m_rideData);
    const TrainingMetricsSummary metrics = TrainingMetrics::compute(
        m_rideData,
        m_ftp,
        m_statistics.durationSeconds);

    m_np  = metrics.normalizedPower;
    m_if  = metrics.intensityFactor;
    m_tss = metrics.trainingStressScore;
    m_vi  = metrics.variabilityIndex;
    m_ef  = metrics.efficiencyFactor;

    ColorContext colorContext;
    colorContext.ftp = static_cast<int>(m_ftp);
    m_powerZones = ZoneCalculator::computeDistribution(
        m_rideData,
        ColorMetric::Power,
        colorContext);
    m_pdcPoints = PowerCurve::computeStandard(m_rideData);

    IntervalDetector::Config cfg;
    cfg.ftp    = m_ftp;
    m_intervals = IntervalDetector::detect(m_rideData, cfg);

    ClimbDetector::Config climbCfg;
    m_climbs = ClimbDetector::detect(m_rideData, climbCfg);
}

void WorkoutController::loadOrGenerateIntervals()
{
    if (!m_dbManager || !m_dbManager->isOpen() || m_currentActivityId <= 0)
        return;

    auto db = m_dbManager->database();
    IntervalRepository repo(db);
    const QList<IntervalRecord> stored = repo.listIntervalsForActivity(m_currentActivityId);

    if (!stored.isEmpty())
    {
        m_intervals.clear();
        m_intervals.reserve(static_cast<size_t>(stored.size()));

        for (const IntervalRecord& record : stored)
        {
            Interval iv;
            iv.startSeconds = record.startSample;
            iv.endSeconds = record.endSample;
            iv.durationSeconds = std::max(0.0, iv.endSeconds - iv.startSeconds);
            iv.averagePower = record.avgPower;
            iv.normalizedPower = record.np;
            iv.averageHeartRate = record.avgHr;
            iv.averageCadence = record.avgCadence;
            iv.label = record.type.isEmpty() ? "Interval" : record.type.toStdString();
            m_intervals.push_back(iv);
        }
        return;
    }

    if (m_intervals.empty())
        return;

    for (const Interval& iv : m_intervals)
    {
        IntervalRecord record;
        record.activityId = m_currentActivityId;
        record.startSample = static_cast<int>(std::round(iv.startSeconds));
        record.endSample = static_cast<int>(std::round(iv.endSeconds));
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
        record.uuid             = UuidGenerator::create();
        repo.insertInterval(record);
    }

    // Mark HasIntervals flag
    {
        auto db2 = m_dbManager->database();
        ActivityRepository actRepo2(db2);
        const Activity actForFlags = actRepo2.getActivity(m_currentActivityId);
        actRepo2.updateAnalysisFlags(m_currentActivityId,
                                      actForFlags.analysisFlags | ActivityAnalysisFlags::HasIntervals);
    }
}

void WorkoutController::reloadClimbsFromDatabase()
{
    if (m_dbManager && m_dbManager->isOpen() && m_currentActivityId > 0)
    {
        m_climbs.clear();
        loadOrGenerateClimbs();
    }
}

void WorkoutController::loadOrGenerateClimbs()
{
    if (!m_dbManager || !m_dbManager->isOpen() || m_currentActivityId <= 0)
        return;

    auto db = m_dbManager->database();
    ClimbRepository repo(db);
    ActivityRepository actRepo(db);

    // Use HasClimbs flag as an authoritative "analysis was ever run" sentinel.
    // This prevents auto re-detect when the user has deliberately deleted all climbs.
    const Activity activity = actRepo.getActivity(m_currentActivityId);
    const bool analysisEverRun = (activity.analysisFlags & ActivityAnalysisFlags::HasClimbs) != 0
                                  || repo.hasClimbsEver(m_currentActivityId);

    if (analysisEverRun)
    {
        // Load non-deleted climbs from DB; rebuild full metrics from ride data.
        const QList<ClimbRecord> stored = repo.climbsForActivity(m_currentActivityId);
        m_climbs.clear();
        m_climbs.reserve(static_cast<size_t>(stored.size()));

        if (m_rideData.records.empty())
            return;

        // Build analysis vectors once with default config
        ClimbDetector::Config cfg;
        const std::vector<double> cumulative = ClimbDetector::buildCumulativeDistanceMeters(m_rideData);
        const std::vector<double> smoothed   = ClimbDetector::smoothAltitudeByDistance(
            m_rideData, cumulative, cfg.elevationSmoothingMeters);
        const std::vector<double> gradient   = ClimbDetector::computeLocalGradientPct(
            smoothed, cumulative, cfg.gradientWindowHalfMeters);

        auto nearestIdx = [this](double sec) -> int
        {
            auto it = std::lower_bound(
                m_rideData.records.begin(), m_rideData.records.end(), sec,
                [](const RideRecord& r, double s) { return r.elapsedSeconds < s; });
            if (it == m_rideData.records.end())
                return static_cast<int>(m_rideData.records.size()) - 1;
            int idx = static_cast<int>(std::distance(m_rideData.records.begin(), it));
            if (idx > 0)
            {
                const double curr = std::abs(m_rideData.records[static_cast<size_t>(idx)].elapsedSeconds - sec);
                const double prev = std::abs(m_rideData.records[static_cast<size_t>(idx - 1)].elapsedSeconds - sec);
                if (prev < curr) --idx;
            }
            return idx;
        };

        for (int ordinal = 0; ordinal < stored.size(); ++ordinal)
        {
            const ClimbRecord& record = stored[ordinal];
            int startIdx = nearestIdx(record.startSeconds);
            int endIdx   = nearestIdx(record.endSeconds);
            if (endIdx < startIdx) std::swap(startIdx, endIdx);
            if (endIdx <= startIdx)
                endIdx = std::min(static_cast<int>(m_rideData.records.size()) - 1, startIdx + 1);

            Climb c = ClimbMetrics::build(m_rideData, cumulative, smoothed, gradient,
                                          startIdx, endIdx, ordinal + 1);
            c.id = record.id;
            if (!record.name.isEmpty())
                c.name = record.name;
            m_climbs.push_back(c);
        }
        return;
    }

    // First time: m_climbs was populated by reanalyze(). Check for resurrection, then persist.
    const QList<ClimbRecord> deletedClimbs = repo.deletedClimbsForActivity(m_currentActivityId);

    for (Climb& c : m_climbs)
    {
        // Resurrection prevention: skip if overlaps a previously deleted climb
        bool overlapsDeleted = false;
        for (const ClimbRecord& d : deletedClimbs)
        {
            if (c.startSeconds < d.endSeconds && c.endSeconds > d.startSeconds)
            {
                overlapsDeleted = true;
                break;
            }
        }
        if (overlapsDeleted) continue;

        ClimbRecord record;
        record.activityId           = m_currentActivityId;
        record.uuid                 = UuidGenerator::create();
        record.startSeconds         = c.startSeconds;
        record.endSeconds           = c.endSeconds;
        record.originalStartSeconds = c.startSeconds;
        record.originalEndSeconds   = c.endSeconds;
        record.name                 = c.name;
        record.elevationGainM       = c.elevationGainM;
        record.lengthKm             = c.lengthKm;
        record.averageGradient      = c.averageGradient;
        record.avgPower             = c.averagePower;
        record.np                   = c.normalizedPower;
        record.avgHr                = c.averageHeartRate;
        record.avgCadence           = c.averageCadence;
        record.vam                  = c.vam;
        record.source               = QStringLiteral("auto");
        record.locked               = false;
        record.algorithmVersion     = AnalysisVersions::ClimbDetection;
        c.id = repo.insertClimb(record);
    }

    // Mark HasClimbs flag
    if (!m_climbs.empty())
        actRepo.updateAnalysisFlags(m_currentActivityId,
                                     activity.analysisFlags | ActivityAnalysisFlags::HasClimbs);
}