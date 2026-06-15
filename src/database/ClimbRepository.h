#pragma once

#include <QList>
#include <QString>

class QSqlDatabase;

/// Full database representation of one stored climb.
struct ClimbRecord
{
    int     id                    = -1;
    int     activityId            = -1;
    QString uuid;                               ///< Stable UUID, generated once on insert.
    double  startSeconds          = 0.0;
    double  endSeconds            = 0.0;
    double  originalStartSeconds  = -1.0;       ///< Bounds at first auto-detection, -1 if not set.
    double  originalEndSeconds    = -1.0;
    QString name;
    double  elevationGainM   = 0.0;
    double  lengthKm         = 0.0;
    double  averageGradient  = 0.0;
    // Cached statistics (avoid recalculation on load)
    double  avgPower         = 0.0;
    double  np               = 0.0;
    double  avgHr            = 0.0;
    double  avgCadence       = 0.0;
    double  vam              = 0.0;
    // Origin / lifecycle
    QString source           = QStringLiteral("auto"); // "auto", "manual", "edited"
    bool    locked           = false;
    bool    deleted          = false;                  ///< Soft-delete flag.
    int     algorithmVersion = 1;
    // Extended
    QString notes;
    bool    favorite = false;
    int     rating   = 0;
};

class ClimbRepository
{
public:
    explicit ClimbRepository(QSqlDatabase& db);

    int  insertClimb(const ClimbRecord& climb);
    bool updateClimb(const ClimbRecord& climb);

    /// Soft-delete: marks deleted=1, locked=1.  Does NOT remove from DB.
    bool softDeleteClimb(int climbId);

    /// Hard-delete a single row.  Use only for internal cleanup.
    bool hardDeleteClimb(int climbId);

    /// Hard-delete all climbs for an activity (full reset before re-detect).
    bool deleteClimbsForActivity(int activityId);

    ClimbRecord        getClimb(int climbId);
    QList<ClimbRecord> climbsForActivity(int activityId);           ///< WHERE deleted=0
    QList<ClimbRecord> deletedClimbsForActivity(int activityId);    ///< WHERE deleted=1

    /// True if any non-deleted climb rows exist.
    bool hasClimbs(int activityId);

    /// True if any non-deleted OR deleted climb rows exist (analysis was ever run).
    bool hasClimbsEver(int activityId);

    /// True if any locked=1 AND deleted=0 climb exists.
    bool hasLockedClimbs(int activityId);

private:
    QSqlDatabase& m_db;
};
