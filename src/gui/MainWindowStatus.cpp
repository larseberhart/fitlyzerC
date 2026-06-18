// SPDX-License-Identifier: GPL-3

#include "MainWindow.h"

#include <QDate>
#include <QFileInfo>

#include "AthleteHeaderWidget.h"
#include "core/settings/DateFormatter.h"
#include "database/ActivityRepository.h"
#include "controllers/ActivityViewController.h"

#include <cmath>

namespace
{
QString formatActivityDateLabel(const QString& isoDate)
{
    const QDate date = QDate::fromString(isoDate, Qt::ISODate);
    if (!date.isValid())
        return QStringLiteral("-");

    const QDate today = QDate::currentDate();
    if (date == today)
        return QStringLiteral("Today");
    if (date == today.addDays(-1))
        return QStringLiteral("Yesterday");

    return DateFormatter::formatDate(date);
}
} // namespace

void MainWindow::updateStatusBarInfo()
{
    if (m_dbStatusLabel)
        m_dbStatusLabel->setText(
            m_dbManager.isOpen()
                ? QString("Database: %1").arg(QFileInfo(m_dbManager.path()).fileName())
                : "Database: none");

    if (m_athleteStatusLabel)
        m_athleteStatusLabel->setText(
            m_currentAthleteName.isEmpty()
                ? "Athlete: none"
                : QString("Athlete: %1").arg(m_currentAthleteName));

    int activityCount = 0;
    if (m_dbManager.isOpen())
    {
        auto db = m_dbManager.database();
        ActivityRepository repo(db);
        activityCount = repo.listActivities(m_currentAthleteId).size();
    }

    if (m_activityCountLabel)
        m_activityCountLabel->setText(QString("Activities: %1").arg(activityCount));

    // Sync athlete context to activity controller for status display
    syncAthleteContextToControllers();
}

void MainWindow::syncAthleteContextToControllers()
{
    if (m_activityViewController)
    {
        m_activityViewController->setAthleteContext(m_currentAthleteId, m_currentAthleteName);
        m_activityViewController->updateStatusBarInformation();
    }
}

void MainWindow::updateStatsLabel()
{
    if (!m_athleteHeader)
        return;

    QString athleteName = m_currentAthleteName.isEmpty() ? "No athlete selected" : m_currentAthleteName;
    int activityCount = 0;
    QString lastActivity = "-";

    if (m_dbManager.isOpen() && m_currentAthleteId > 0)
    {
        auto db = m_dbManager.database();
        ActivityRepository actRepo(db);
        const QList<Activity> activities = actRepo.listActivities(m_currentAthleteId);
        activityCount = activities.size();
        if (!activities.isEmpty())
        {
            const QString rawDate = !activities.first().startTime.isEmpty()
                ? activities.first().startTime.left(10)
                : activities.first().importedAt.left(10);
            lastActivity = formatActivityDateLabel(rawDate);
        }
    }

    const int ftpWatts = static_cast<int>(m_controller->ftp());
    const double estimatedFtp = estimatedFtpFromCurrentRide();
    const int estimatedFtpRounded = estimatedFtp > 0.0 ? static_cast<int>(std::round(estimatedFtp)) : 0;
    const int ftpDelta = estimatedFtpRounded - ftpWatts;
    m_athleteHeader->setSummary(athleteName, ftpWatts, activityCount, lastActivity);

    if (m_useEstimatedFtpButton)
        m_useEstimatedFtpButton->setEnabled(m_currentAthleteId > 0 && estimatedFtpRounded > 0);

    const QString ftpSummary = estimatedFtpRounded > 0
        ? QString("FTP %1  Est %2  \u0394%3")
              .arg(ftpWatts)
              .arg(estimatedFtpRounded)
              .arg(ftpDelta >= 0 ? QString("+%1").arg(ftpDelta) : QString::number(ftpDelta))
        : QString();

    const QString rideText = QString("NP %1 W   IF %2   TSS %3   VI %4   EF %5%6")
        .arg(m_controller->normalizedPower(), 0, 'f', 0)
        .arg(m_controller->intensityFactor(), 0, 'f', 2)
        .arg(m_controller->trainingStressScore(), 0, 'f', 0)
        .arg(m_controller->variabilityIndex(), 0, 'f', 2)
        .arg(m_controller->efficiencyFactor(), 0, 'f', 2)
        .arg(ftpSummary.isEmpty() ? QString() : QString("   ") + ftpSummary);
    m_athleteHeader->setRideMetrics(rideText);
}
