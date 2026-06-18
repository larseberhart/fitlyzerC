// SPDX-License-Identifier: GPL-3

#include "MainWindow.h"

#include <QFileInfo>

#include "database/ActivityRepository.h"
#include "controllers/ActivityViewController.h"

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
