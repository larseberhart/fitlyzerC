// SPDX-License-Identifier: GPL-3

/**
 * @file WorkoutController.h
 * @brief User interface component for WorkoutController.
 *
 * Defines dialogs, widgets, controllers, and UI workflows used by the FitlyzerC desktop application.
 *
 * Responsibilities:
 * - Provide interactive user interface behavior and presentation
 *
 * @author Lars EBERHART
 */

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

/**
 * @brief Activity import/export result status.
 */
enum class ImportResult
{
    Success,
    Duplicate,
    TimeOverlap,
    Failed
};

/**
 * @brief Information about a duplicate activity detection.
 */
struct DuplicateActivityInfo
{
    /// @brief ID of the existing (conflicting) activity.
    int existingActivityId = -1;

    /// @brief Start time of existing activity in UTC ISO format.
    QString existingStartUtc;

    /// @brief Duration of existing activity in seconds.
    double existingDurationSeconds = 0.0;

    /// @brief Start time of new (import) activity in UTC ISO format.
    QString newStartUtc;

    /// @brief Duration of new (import) activity in seconds.
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
    /**
     * @brief Constructs workout controller.
     * @param parent Parent object.
     */
    explicit WorkoutController(QObject* parent = nullptr);

    /**
     * @brief Sets functional threshold power.
     * @param ftp FTP in watts.
     */
    void   setFTP(double ftp);

    /**
     * @brief Gets functional threshold power.
     * @return FTP in watts.
     */
    double ftp() const { return m_ftp; }

    /**
     * @brief Sets database manager.
     * @param dbManager Database manager.
     */
    void setDatabaseManager(DatabaseManager* dbManager);

    /**
     * @brief Sets current athlete.
     * @param athleteId Athlete ID.
     */
    void setCurrentAthlete(int athleteId);

    /**
     * @brief Gets current athlete ID.
     * @return Athlete ID or -1 if none.
     */
    int  currentAthleteId() const { return m_currentAthleteId; }

    /**
     * @brief Loads ride data from FIT file into memory.
     * @param path File path.
     * @param errorOut Error message if failed.
     * @return True on success.
     */
    bool loadFile(const QString& path, QString& errorOut);

    /**
     * @brief Imports FIT file to database.
     * @param path File path.
     * @param errorOut Error message if failed.
     * @param allowTimeOverlap Allow overlapping times (default false).
     * @param runAnalysis Run analysis after import (default true).
     * @param importSource Source identifier.
     * @param duplicateInfo Duplicate info if detected.
     * @param result Import result status.
     * @return Activity ID or -1 on failure.
     */
    int  importFile(const QString& path, QString& errorOut,
                    bool allowTimeOverlap = false,
                    bool runAnalysis = true,
                    const QString& importSource = QString(),
                    DuplicateActivityInfo* duplicateInfo = nullptr,
                    ImportResult* result = nullptr);

    /**
     * @brief Loads activity from database.
     * @param activityId Activity ID.
     * @param errorOut Error message if failed.
     * @return True on success.
     */
    bool loadActivity(int activityId, QString& errorOut);

    /**
     * @brief Reloads climbs from database after edits.
     */
    void reloadClimbsFromDatabase();

    /**
     * @brief Sets analysis queue for background processing.
     * @param queue Analysis queue.
     */
    void setAnalysisQueue(AnalysisQueue* queue) { m_analysisQueue = queue; }

    /**
     * @brief Gets loaded ride data.
     * @return Ride data.
     */
    const RideData&                      rideData()    const { return m_rideData; }

    /**
     * @brief Gets workout statistics.
     * @return Statistics.
     */
    const WorkoutStatistics&             statistics()  const { return m_statistics; }

    /**
     * @brief Gets power zone distribution.
     * @return Zone distribution.
     */
    const std::vector<ZoneDistribution>& powerZones()  const { return m_powerZones; }

    /**
     * @brief Gets power duration curve points.
     * @return Power curve points.
     */
    const std::vector<PowerCurvePoint>&  pdcPoints()   const { return m_pdcPoints; }

    /**
     * @brief Gets detected intervals.
     * @return Intervals.
     */
    const std::vector<Interval>&         intervals()   const { return m_intervals; }

    /**
     * @brief Gets detected climbs.
     * @return Climbs.
     */
    const std::vector<Climb>&            climbs()      const { return m_climbs; }

    /**
     * @brief Gets normalized power.
     * @return Normalized power in watts.
     */
    double normalizedPower()     const { return m_np;  }

    /**
     * @brief Gets intensity factor.
     * @return Intensity factor (0-1).
     */
    double intensityFactor()     const { return m_if;  }

    /**
     * @brief Gets training stress score.
     * @return TSS value.
     */
    double trainingStressScore() const { return m_tss; }

    /**
     * @brief Gets variability index.
     * @return Variability index.
     */
    double variabilityIndex()    const { return m_vi;  }

    /**
     * @brief Gets efficiency factor.
     * @return Efficiency factor.
     */
    double efficiencyFactor()    const { return m_ef;  }

    /**
     * @brief Gets current activity ID.
     * @return Activity ID or -1 if none.
     */
    int currentActivityId() const { return m_currentActivityId; }

signals:
    /// \signal Emitted when activity finishes loading.
    void workoutLoaded();

    /// \signal Emitted when activity imported.
    /// \param activityId ID of imported activity.
    void activityImported(int activityId);

    /// \signal Emitted when FTP changes.
    /// \param ftp New FTP in watts.
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