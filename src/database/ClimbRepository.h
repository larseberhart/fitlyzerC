#pragma once

#include <QList>
#include <QString>

class QSqlDatabase;

// Lightweight DB record for a stored climb boundary + metadata.
// Full metrics (power, HR, VAM, etc.) are recomputed from ride data on load.
struct ClimbRecord
{
    int    id            = -1;
    int    activityId    = -1;
    double startSeconds  = 0.0;
    double endSeconds    = 0.0;
    QString name;
    double elevationGainM    = 0.0;
    double lengthKm          = 0.0;
    double averageGradient   = 0.0;
    QString source = QStringLiteral("auto"); // "auto", "manual", "edited"
    bool   locked  = false;
};

class ClimbRepository
{
public:
    explicit ClimbRepository(QSqlDatabase& db);

    int  insertClimb(const ClimbRecord& climb);
    bool updateClimb(const ClimbRecord& climb);
    bool deleteClimb(int climbId);
    bool deleteClimbsForActivity(int activityId);

    QList<ClimbRecord> climbsForActivity(int activityId);
    bool hasClimbs(int activityId);
    bool hasLockedClimbs(int activityId);

private:
    QSqlDatabase& m_db;
};
