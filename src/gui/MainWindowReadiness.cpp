// SPDX-License-Identifier: GPL-3

#include "MainWindow.h"

#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QStatusBar>

#include "AthleteDialog.h"
#include "WelcomeWidget.h"
#include "database/ActivityRepository.h"
#include "database/AthleteRepository.h"
#include "platform/Platform.h"

bool MainWindow::canImportActivities() const
{
    return m_dbManager.isOpen() && m_currentAthleteId > 0;
}

bool MainWindow::createOrOpenDefaultDatabase()
{
    const QString path = defaultDatabasePathForAutoCreate();
    if (path.isEmpty())
        return false;

    const QFileInfo info(path);
    QDir dir = info.dir();
    if (!dir.exists() && !dir.mkpath("."))
    {
        QMessageBox::critical(this, "Database Error",
            "Could not create default database folder:\n" + dir.absolutePath());
        return false;
    }

    QString err;
    const bool ok = info.exists()
        ? m_dbManager.open(path, &err)
        : m_dbManager.create(path, &err);
    if (!ok)
    {
        QMessageBox::critical(this, "Database Error",
            QString("Could not open default database:\n%1\n\n%2").arg(path, err));
        return false;
    }

    m_controller->setDatabaseManager(&m_dbManager);
    m_lastOpenDir = info.absolutePath();
    if (m_analysisQueue)
        m_analysisQueue->setDatabasePath(path);
    if (m_importQueue)
        m_importQueue->setDatabasePath(path);

    QSettings settings("Fitlyzer", "FitlyzerC");
    settings.setValue("lastDatabase", path);
    addRecentDatabase(path);
    updateRecentDatabaseMenu();
    return true;
}

bool MainWindow::ensureDatabase()
{
    if (m_dbManager.isOpen())
        return true;

    if (!createOrOpenDefaultDatabase())
        return false;

    refreshAthleteSelector();
    updateAthleteLabel();
    updateImportAvailability();
    updateStatsLabel();
    updateStatusBarInfo();
    updateWelcomeScreenVisibility();

    statusBar()->showMessage("Created default database for import.", 3500);
    return true;
}

bool MainWindow::ensureAthlete()
{
    if (!m_dbManager.isOpen())
        return false;

    auto db = m_dbManager.database();
    AthleteRepository repo(db);
    QList<Athlete> athletes = repo.listAthletes();

    bool createdDefaultAthlete = false;
    if (athletes.isEmpty())
    {
        Athlete athlete;
        athlete.firstName = "Default";
        athlete.lastName = "Athlete";
        const int createdId = repo.insertAthlete(athlete);
        if (createdId <= 0)
        {
            QMessageBox::critical(this, "Athlete Error",
                "Could not create a default athlete for import.");
            return false;
        }
        athletes = repo.listAthletes();
        createdDefaultAthlete = true;
    }

    bool hasCurrent = false;
    for (const Athlete& athlete : athletes)
    {
        if (athlete.id == m_currentAthleteId)
        {
            hasCurrent = true;
            break;
        }
    }

    if (!hasCurrent)
    {
        m_currentAthleteId = athletes.first().id;
        m_currentAthleteName = athletes.first().fullName();
        m_controller->setCurrentAthlete(m_currentAthleteId);
        QSettings("Fitlyzer", "FitlyzerC").setValue("currentAthleteId", m_currentAthleteId);
    }

    refreshAthleteSelector();
    updateAthleteLabel();
    updateImportAvailability();
    updateStatsLabel();
    updateStatusBarInfo();

    if (createdDefaultAthlete)
    {
        const auto ans = QMessageBox::question(
            this,
            "Default Athlete Created",
            "A default athlete was created. Rename athlete now?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);

        if (ans == QMessageBox::Yes)
        {
            AthleteDialog dlg(repo, m_currentAthleteId, this);
            if (dlg.exec() == QDialog::Accepted)
            {
                const Athlete athlete = repo.getAthlete(m_currentAthleteId);
                m_currentAthleteName = athlete.fullName();
                refreshAthleteSelector();
                updateAthleteLabel();
                updateStatusBarInfo();
            }
        }
    }

    return m_currentAthleteId > 0;
}

bool MainWindow::ensureImportReady()
{
    return ensureDatabase() && ensureAthlete();
}

QString MainWindow::defaultDatabasePathForAutoCreate() const
{
    const QString documentsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString baseDir = documentsDir.isEmpty()
        ? Platform::defaultDatabaseDirectory()
        : documentsDir;
    return QDir(baseDir).filePath("Fitlyzer/default.fitlyzerdb");
}

void MainWindow::updateImportAvailability()
{
    const bool enabled = canImportActivities();
    if (m_importAct)
        m_importAct->setEnabled(enabled);
    if (m_toolbarImportAct)
        m_toolbarImportAct->setEnabled(enabled);

    if (enabled)
    {
        statusBar()->showMessage("Ready to import activities.", 2000);
    }
    else if (!m_dbManager.isOpen())
    {
        statusBar()->showMessage("Open or create a database to import activities.", 3000);
    }
    else
    {
        statusBar()->showMessage("Select an athlete to import activities.", 3000);
    }
}

void MainWindow::showWelcomeScreen()
{
    if (m_welcomeWidget)
        m_welcomeWidget->setVisible(true);
    if (m_tabWidget)
        m_tabWidget->setVisible(false);
}

void MainWindow::hideWelcomeScreen()
{
    if (m_welcomeWidget)
        m_welcomeWidget->setVisible(false);
    if (m_tabWidget)
        m_tabWidget->setVisible(true);
}

int MainWindow::activityCount() const
{
    if (!m_dbManager.isOpen())
        return 0;

    auto db = m_dbManager.database();
    ActivityRepository repo(db);
    return repo.listActivities(-1).size();
}

void MainWindow::updateWelcomeScreenVisibility()
{
    if (!m_welcomeWidget)
        return;

    m_welcomeWidget->setFirstLaunch(!m_firstLaunchCompleted);
    if (activityCount() == 0)
        showWelcomeScreen();
    else
        hideWelcomeScreen();
}
