// SPDX-License-Identifier: GPL-3

/**
 * @file AthleteRepository.h
 * @brief Database access component for AthleteRepository.
 *
 * Implements database schema handling, repository operations, or SQL utility behavior for persistent storage.
 *
 * Responsibilities:
 * - Provide database schema, query, or repository functionality
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QDate>
#include <QList>
#include <QString>

class QSqlDatabase;

/**
 * @brief Represents an athlete profile.
 */
struct Athlete
{
    /// @brief Unique athlete identifier.
    int     id         = -1;

    /// @brief Athlete first name.
    QString firstName;

    /// @brief Athlete last name.
    QString lastName;

    /// @brief Date of birth (ISO 8601 YYYY-MM-DD).
    QString dateOfBirth;

    /// @brief Email address.
    QString email;

    /// @brief Height in centimeters.
    double  heightCm   = 0.0;

    /// @brief Functional threshold power in watts.
    int     ftpWatts   = 250;

    /// @brief ISO8601 creation timestamp.
    QString createdAt;

    /**
     * @brief Returns full name (first + space + last).
     */
    QString fullName() const { return firstName + " " + lastName; }

    /**
     * @brief Checks if athlete data is valid.
     */
    bool    isValid()  const { return id > 0; }
};

/**
 * @brief Represents a historical FTP entry.
 */
struct FtpEntry
{
    /// @brief Unique FTP entry identifier.
    int     id            = -1;

    /// @brief Associated athlete ID.
    int     athleteId     = -1;

    /// @brief FTP value in watts.
    int     ftpWatts      = 0;

    /// @brief Date FTP becomes effective (ISO 8601 YYYY-MM-DD).
    QString effectiveFrom;

    /// @brief Optional notes about FTP change.
    QString notes;
};

/**
 * @brief Represents a historical weight entry.
 */
struct WeightEntry
{
    /// @brief Unique weight entry identifier.
    int     id            = -1;

    /// @brief Associated athlete ID.
    int     athleteId     = -1;

    /// @brief Body weight in kilograms.
    double  weightKg      = 0.0;

    /// @brief Date weight becomes effective (ISO 8601 YYYY-MM-DD).
    QString effectiveFrom;
};

/**
 * @brief Repository for athlete profile data persistence.
 *
 * Provides CRUD operations for athletes and their FTP/weight history.
 */
class AthleteRepository
{
public:
    /**
     * @brief Constructs repository with database connection.
     * @param db Database connection.
     */
    explicit AthleteRepository(QSqlDatabase& db);

    /**
     * @brief Inserts a new athlete.
     * @param a Athlete to insert.
     * @return ID of inserted athlete, or -1 on error.
     */
    int             insertAthlete(const Athlete& a);

    /**
     * @brief Updates an existing athlete.
     * @param a Athlete with updated values.
     * @return True if update succeeded.
     */
    bool            updateAthlete(const Athlete& a);

    /**
     * @brief Deletes an athlete.
     * @param athleteId Athlete ID.
     * @return True if deletion succeeded.
     */
    bool            deleteAthlete(int athleteId);

    /**
     * @brief Lists all athletes.
     * @return List of athlete records.
     */
    QList<Athlete>  listAthletes();

    /**
     * @brief Retrieves an athlete by ID.
     * @param athleteId Athlete ID.
     * @return Athlete record.
     */
    Athlete         getAthlete(int athleteId);

    /**
     * @brief Adds an FTP history entry.
     * @param e FTP entry to add.
     * @return ID of inserted entry, or -1 on error.
     */
    int              addFtpEntry(const FtpEntry& e);

    /**
     * @brief Deletes an FTP history entry.
     * @param entryId Entry ID.
     * @return True if deletion succeeded.
     */
    bool             deleteFtpEntry(int entryId);

    /**
     * @brief Retrieves FTP history for an athlete.
     * @param athleteId Athlete ID.
     * @return List of FTP entries.
     */
    QList<FtpEntry>  getFtpHistory(int athleteId);

    /**
     * @brief Gets FTP for an athlete on a specific date.
     * @param athleteId Athlete ID.
     * @param date Query date.
     * @return FTP in watts, or 0 if no entry found on or before date.
     */
    int              getFtpForDate(int athleteId, const QDate& date);

    /**
     * @brief Adds a weight history entry.
     * @param e Weight entry to add.
     * @return ID of inserted entry, or -1 on error.
     */
    int               addWeightEntry(const WeightEntry& e);

    /**
     * @brief Deletes a weight history entry.
     * @param entryId Entry ID.
     * @return True if deletion succeeded.
     */
    bool              deleteWeightEntry(int entryId);

    /**
     * @brief Retrieves weight history for an athlete.
     * @param athleteId Athlete ID.
     * @return Weight entries sorted by date.
     */
    QList<WeightEntry> getWeightHistory(int athleteId);

private:
    QSqlDatabase& m_db;
};