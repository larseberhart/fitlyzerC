// SPDX-License-Identifier: GPL-3


#include "AthleteRepository.h"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

AthleteRepository::AthleteRepository(QSqlDatabase& db)
    : m_db(db)
{}

// ── Athletes ──────────────────────────────────────────────────────────────────

int AthleteRepository::insertAthlete(const Athlete& a)
{
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO athletes(first_name, last_name, date_of_birth, email, height_cm, ftp, created_at)"
        " VALUES(:fn, :ln, :dob, :email, :h, :ftp, :ts)");
    q.bindValue(":fn",    a.firstName);
    q.bindValue(":ln",    a.lastName);
    q.bindValue(":dob",   a.dateOfBirth);
    q.bindValue(":email", a.email);
    q.bindValue(":h",     a.heightCm > 0.0 ? QVariant(a.heightCm) : QVariant());
    q.bindValue(":ftp",   a.ftpWatts > 0 ? a.ftpWatts : 250);
    q.bindValue(":ts",    QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    if (!q.exec()) return -1;
    return q.lastInsertId().toInt();
}

bool AthleteRepository::updateAthlete(const Athlete& a)
{
    QSqlQuery q(m_db);
    q.prepare(
        "UPDATE athletes SET first_name=:fn, last_name=:ln, date_of_birth=:dob,"
        " email=:email, height_cm=:h, ftp=:ftp WHERE id=:id");
    q.bindValue(":fn",    a.firstName);
    q.bindValue(":ln",    a.lastName);
    q.bindValue(":dob",   a.dateOfBirth);
    q.bindValue(":email", a.email);
    q.bindValue(":h",     a.heightCm > 0.0 ? QVariant(a.heightCm) : QVariant());
    q.bindValue(":ftp",   a.ftpWatts > 0 ? a.ftpWatts : 250);
    q.bindValue(":id",    a.id);
    return q.exec();
}

bool AthleteRepository::deleteAthlete(int athleteId)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM athletes WHERE id=:id");
    q.bindValue(":id", athleteId);
    return q.exec();
}

QList<Athlete> AthleteRepository::listAthletes()
{
    QList<Athlete> result;
    QSqlQuery q(m_db);
    q.exec("SELECT id, first_name, last_name, date_of_birth, email, height_cm, ftp, created_at"
           " FROM athletes ORDER BY last_name, first_name");
    while (q.next())
    {
        Athlete a;
        a.id          = q.value(0).toInt();
        a.firstName   = q.value(1).toString();
        a.lastName    = q.value(2).toString();
        a.dateOfBirth = q.value(3).toString();
        a.email       = q.value(4).toString();
        a.heightCm    = q.value(5).toDouble();
        a.ftpWatts    = q.value(6).toInt() > 0 ? q.value(6).toInt() : 250;
        a.createdAt   = q.value(7).toString();
        result.append(a);
    }
    return result;
}

Athlete AthleteRepository::getAthlete(int athleteId)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT id, first_name, last_name, date_of_birth, email, height_cm, ftp, created_at"
              " FROM athletes WHERE id=:id");
    q.bindValue(":id", athleteId);
    q.exec();
    if (q.next())
    {
        Athlete a;
        a.id          = q.value(0).toInt();
        a.firstName   = q.value(1).toString();
        a.lastName    = q.value(2).toString();
        a.dateOfBirth = q.value(3).toString();
        a.email       = q.value(4).toString();
        a.heightCm    = q.value(5).toDouble();
        a.ftpWatts    = q.value(6).toInt() > 0 ? q.value(6).toInt() : 250;
        a.createdAt   = q.value(7).toString();
        return a;
    }
    return {};
}

// ── FTP history ───────────────────────────────────────────────────────────────

int AthleteRepository::addFtpEntry(const FtpEntry& e)
{
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO ftp_history(athlete_id, ftp_watts, effective_from, notes)"
        " VALUES(:aid, :w, :from, :notes)");
    q.bindValue(":aid",   e.athleteId);
    q.bindValue(":w",     e.ftpWatts);
    q.bindValue(":from",  e.effectiveFrom);
    q.bindValue(":notes", e.notes);
    if (!q.exec()) return -1;
    return q.lastInsertId().toInt();
}

bool AthleteRepository::deleteFtpEntry(int entryId)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM ftp_history WHERE id=:id");
    q.bindValue(":id", entryId);
    return q.exec();
}

QList<FtpEntry> AthleteRepository::getFtpHistory(int athleteId)
{
    QList<FtpEntry> result;
    QSqlQuery q(m_db);
    q.prepare("SELECT id, athlete_id, ftp_watts, effective_from, notes"
              " FROM ftp_history WHERE athlete_id=:aid ORDER BY effective_from DESC");
    q.bindValue(":aid", athleteId);
    q.exec();
    while (q.next())
    {
        FtpEntry e;
        e.id            = q.value(0).toInt();
        e.athleteId     = q.value(1).toInt();
        e.ftpWatts      = q.value(2).toInt();
        e.effectiveFrom = q.value(3).toString();
        e.notes         = q.value(4).toString();
        result.append(e);
    }
    return result;
}

int AthleteRepository::getFtpForDate(int athleteId, const QDate& date)
{
    // Find the most recent FTP entry on or before the given date.
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT ftp_watts FROM ftp_history"
        " WHERE athlete_id=:aid AND effective_from <= :date"
        " ORDER BY effective_from DESC LIMIT 1");
    q.bindValue(":aid",  athleteId);
    q.bindValue(":date", date.toString(Qt::ISODate));
    q.exec();
    if (q.next())
        return q.value(0).toInt();
    return 0;
}

// ── Weight history ────────────────────────────────────────────────────────────

int AthleteRepository::addWeightEntry(const WeightEntry& e)
{
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO weight_history(athlete_id, weight_kg, effective_from)"
        " VALUES(:aid, :w, :from)");
    q.bindValue(":aid",  e.athleteId);
    q.bindValue(":w",    e.weightKg);
    q.bindValue(":from", e.effectiveFrom);
    if (!q.exec()) return -1;
    return q.lastInsertId().toInt();
}

bool AthleteRepository::deleteWeightEntry(int entryId)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM weight_history WHERE id=:id");
    q.bindValue(":id", entryId);
    return q.exec();
}

QList<WeightEntry> AthleteRepository::getWeightHistory(int athleteId)
{
    QList<WeightEntry> result;
    QSqlQuery q(m_db);
    q.prepare("SELECT id, athlete_id, weight_kg, effective_from"
              " FROM weight_history WHERE athlete_id=:aid ORDER BY effective_from DESC");
    q.bindValue(":aid", athleteId);
    q.exec();
    while (q.next())
    {
        WeightEntry e;
        e.id            = q.value(0).toInt();
        e.athleteId     = q.value(1).toInt();
        e.weightKg      = q.value(2).toDouble();
        e.effectiveFrom = q.value(3).toString();
        result.append(e);
    }
    return result;
}