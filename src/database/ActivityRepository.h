#pragma once

#include <QList>
#include <QString>

class QSqlDatabase;

// ── Data structures ──────────────────────────────────────────────────────────

struct Activity
{
    int     id              = -1;
    int     athleteId       = -1;
    QString fitHash;
    QString fileName;
    QString sport;
    QString startTime;
    QString endTime;
    double  durationSec     = 0.0;
    double  distanceM       = 0.0;
    double  avgPower        = 0.0;
    double  maxPower        = 0.0;
    double  normalizedPower = 0.0;
    double  avgHr           = 0.0;
    double  maxHr           = 0.0;
    double  avgCadence      = 0.0;
    double  avgSpeed        = 0.0;
    double  elevationGain   = 0.0;
    QString notes;
    QString weatherNotes;
    double  temperature     = 0.0;
    QString weather;
    QString wind;
    int     rpe             = 0;
    int     fatigue         = 0;
    double  sleepHours      = 0.0;
    double  weightKg        = 0.0;
    QString bike;
    QString equipment;
    QString importedAt;

    bool isValid() const { return id > 0; }
};

// ── Repository ───────────────────────────────────────────────────────────────

class ActivityRepository
{
public:
    explicit ActivityRepository(QSqlDatabase& db);

    int            insertActivity(const Activity& a);
    Activity       getActivity(int activityId);
    QList<Activity> listActivities(int athleteId);
    bool           deleteActivity(int activityId);

    // Duplicate detection via SHA-256 of file contents.
    bool           hasFitHash(const QString& hash);

    bool           updateNotes(int activityId, const QString& notes,
                               const QString& weatherNotes);
    bool           updateActivityProperties(int activityId, const Activity& a);

private:
    QSqlDatabase& m_db;
};
