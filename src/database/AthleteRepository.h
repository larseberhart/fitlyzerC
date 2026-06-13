#pragma once

#include <QDate>
#include <QList>
#include <QString>

class QSqlDatabase;

// ── Data structures ──────────────────────────────────────────────────────────

struct Athlete
{
    int     id         = -1;
    QString firstName;
    QString lastName;
    QString dateOfBirth;  // ISO 8601 date string "YYYY-MM-DD"
    QString email;
    double  heightCm   = 0.0;
    int     ftpWatts   = 250;
    QString createdAt;

    QString fullName() const { return firstName + " " + lastName; }
    bool    isValid()  const { return id > 0; }
};

struct FtpEntry
{
    int     id            = -1;
    int     athleteId     = -1;
    int     ftpWatts      = 0;
    QString effectiveFrom; // ISO date "YYYY-MM-DD"
    QString notes;
};

struct WeightEntry
{
    int     id            = -1;
    int     athleteId     = -1;
    double  weightKg      = 0.0;
    QString effectiveFrom; // ISO date "YYYY-MM-DD"
};

// ── Repository ───────────────────────────────────────────────────────────────

class AthleteRepository
{
public:
    explicit AthleteRepository(QSqlDatabase& db);

    // Athletes
    int             insertAthlete(const Athlete& a);
    bool            updateAthlete(const Athlete& a);
    bool            deleteAthlete(int athleteId);
    QList<Athlete>  listAthletes();
    Athlete         getAthlete(int athleteId);

    // FTP history
    int              addFtpEntry(const FtpEntry& e);
    bool             deleteFtpEntry(int entryId);
    QList<FtpEntry>  getFtpHistory(int athleteId);
    // Returns 0 if no entry exists on or before the given date.
    int              getFtpForDate(int athleteId, const QDate& date);

    // Weight history
    int               addWeightEntry(const WeightEntry& e);
    bool              deleteWeightEntry(int entryId);
    QList<WeightEntry> getWeightHistory(int athleteId);

private:
    QSqlDatabase& m_db;
};
