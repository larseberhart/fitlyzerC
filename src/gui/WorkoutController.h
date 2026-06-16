// SPDX-License-Identifier: GPL-3


#pragma once

#include <QDate>
#include <QObject>
#include <QString>

#include "analysis/IntervalDetector.h"
#include "analysis/ClimbDetector.h"
#include "analysis/PowerCurve.h"
#include "analysis/TrainingLoad.h"
#include "analysis/TrainingMetrics.h"
#include "analysis/WorkoutAnalyzer.h"
#include "core/zones/ZoneDefinition.h"
#include "analysis/queue/AnalysisQueue.h"
#include "core/AnalysisVersions.h"
#include "core/ActivityAnalysisFlags.h"
#include "database/ActivityRepository.h"
#include "database/AthleteRepository.h"
#include "database/ClimbRepository.h"
#include "database/ImportRepository.h"
#include "database/IntervalRepository.h"
#include "fit/RideData.h"

class DatabaseManager;

enum class ImportResult
{
    Success,
    Duplicate,
    TimeOverlap,
    Failed
};

struct DuplicateActivityInfo
{
    // Existing conflicting activity ID.
    int existingActivityId = -1;

    // Existing activity start time in UTC ISO format.
    QString existingStartUtc;

    // Existing activity duration in seconds.
    double existingDurationSeconds = 0.0;

    // Incoming activity start time in UTC ISO format.
    QString newStartUtc;

    // Incoming activity duration in seconds.
    double newDurationSeconds = 0.0;
};

/**
 * @brief Manages activity loading, import, and analysis.
 *
 * Central controller for ride data loading, importing FIT files, and coordinating
 * analysis operations including climb detection, interval detection, and metrics computation.
 */
class WorkoutController : public QObject
{
    Q_OBJECT
public:
    explicit WorkoutController(QObject* parent = nullptr);

    void   setFTP(double ftp);

    double ftp() const { return m_ftp; }

    void setDatabaseManager(DatabaseManager* dbManager);

    void setCurrentAthlete(int athleteId);

    int  currentAthleteId() const { return m_currentAthleteId; }

    bool loadFile(const QString& path, QString& errorOut);

    int  importFile(const QString& path, QString& errorOut,
                    bool allowTimeOverlap = false,
                    bool runAnalysis = true,
                    const QString& importSource = QString(),
                    DuplicateActivityInfo* duplicateInfo = nullptr,
                    ImportResult* result = nullptr);

    bool loadActivity(int activityId, QString& errorOut);

    void reloadClimbsFromDatabase();

    void setAnalysisQueue(AnalysisQueue* queue) { m_analysisQueue = queue; }

    const RideData&                      rideData()    const { return m_rideData; }

    const WorkoutStatistics&             statistics()  const { return m_statistics; }

    const std::vector<ZoneDistribution>& powerZones()  const { return m_powerZones; }

    const std::vector<PowerCurvePoint>&  pdcPoints()   const { return m_pdcPoints; }

    const std::vector<Interval>&         intervals()   const { return m_intervals; }

    const std::vector<Climb>&            climbs()      const { return m_climbs; }

    double normalizedPower()     const { return m_np;  }

    double intensityFactor()     const { return m_if;  }

    double trainingStressScore() const { return m_tss; }

    double variabilityIndex()    const { return m_vi;  }

    double efficiencyFactor()    const { return m_ef;  }

    int currentActivityId() const { return m_currentActivityId; }

signals:
    // Emitted when activity finishes loading.
    void workoutLoaded();

    // Emitted when activity is imported.
    void activityImported(int activityId);

    // Emitted when FTP changes.
    void ftpChanged(double ftp);

private:
    void reanalyze();
    void loadOrGenerateIntervals();
    void loadOrGenerateClimbs();
    AnalysisQueue* m_analysisQueue = nullptr;

    double m_ftp               = 250.0;
    int    m_currentAthleteId  = -1;
    int    m_currentActivityId = -1;

    DatabaseManager* m_dbManager = nullptr;

    RideData                     m_rideData;
    WorkoutStatistics            m_statistics;
    WorkoutAnalyzer              m_analyzer;
    std::vector<ZoneDistribution> m_powerZones;
    std::vector<PowerCurvePoint> m_pdcPoints;
    std::vector<Interval>        m_intervals;
    std::vector<Climb>           m_climbs;

    double m_np  = 0.0;
    double m_if  = 0.0;
    double m_tss = 0.0;
    double m_vi  = 0.0;
    double m_ef  = 0.0;
};