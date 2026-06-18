// SPDX-License-Identifier: GPL-3

#include "MainWindow.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QStatusBar>

#include "ActivityBrowser.h"
#include "CalendarWidget.h"
#include "CreateDatabaseDialog.h"
#include "database/AthleteRepository.h"
#include "maps/TileCache.h"
#include "platform/Platform.h"

namespace
{
/// @brief Maximum number of recent databases to remember.
static constexpr int kMaxRecentDatabases = 8;

/// @brief Default chart preset identifier value.
static constexpr int kChartPresetCustom = 0;
}

void MainWindow::saveSettings()
{
    QSettings s("Fitlyzer", "FitlyzerC");
    s.setValue("geometry",           saveGeometry());
    s.setValue("activeTab",          m_tabWidget->currentIndex());
    if (m_analysisTabWidget)
        s.setValue("analysisTab",    m_analysisTabWidget->currentIndex());
    if (m_colorMetricCombo)
        s.setValue("colorMetric",    m_colorMetricCombo->currentData().toInt());
    if (m_powerSmoothingCombo)
        s.setValue("powerSmoothing", m_powerSmoothingCombo->currentData().toInt());
    if (m_chartPresetCombo)
        s.setValue("chartPreset", m_chartPresetCombo->currentData().toInt());
    if (m_autoSmoothingCheck)
        s.setValue("autoSmoothing",  m_autoSmoothingCheck->isChecked());
    if (m_followIntervalSelectionCheck)
        s.setValue("followIntervalSelection", m_followIntervalSelectionCheck->isChecked());
    if (m_mapStyleCombo)
        s.setValue("mapStyle", m_mapStyleCombo->currentData().toInt());
    if (m_mapAutoContrastCheck)
        s.setValue("mapAutoRouteContrast", m_mapAutoContrastCheck->isChecked());
    s.setValue("lastOpenDir",        m_lastOpenDir);
    s.setValue("showPower",          m_showPower->isChecked());
    s.setValue("showHR",             m_showHR->isChecked());
    s.setValue("showCadence",        m_showCadence->isChecked());
    s.setValue("showSpeed",          m_showSpeed->isChecked());
    s.setValue("showAltitude",       m_showAltitude->isChecked());
    s.setValue("watchFolderEnabled", m_watchFolderEnabled);
    s.setValue("watchFolderPath",    m_watchFolderPath);
    s.setValue("chartHeight",        m_chartHeight);
    s.setValue("currentAthleteId",   m_currentAthleteId);
    s.setValue("lastActivityId",     m_controller ? m_controller->currentActivityId() : -1);
    if (m_dbManager.isOpen())
        s.setValue("lastDatabase",   m_dbManager.path());
}

void MainWindow::loadSettings()
{
    QSettings s("Fitlyzer", "FitlyzerC");

    if (s.contains("geometry"))
        restoreGeometry(s.value("geometry").toByteArray());

    m_lastOpenDir = s.value("lastOpenDir", QString()).toString();

    m_showPower->setChecked(s.value("showPower",    true).toBool());
    m_showHR->setChecked(   s.value("showHR",       true).toBool());
    m_showCadence->setChecked(s.value("showCadence",true).toBool());
    m_showSpeed->setChecked(  s.value("showSpeed",  true).toBool());
    m_showAltitude->setChecked(s.value("showAltitude", true).toBool());
    m_watchFolderEnabled = s.value("watchFolderEnabled", true).toBool();
    m_watchFolderPath = s.value(
        "watchFolderPath",
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();

    m_chartHeight = std::clamp(s.value("chartHeight", 220).toInt(), 120, 600);

    if (m_powerSmoothingCombo)
    {
        const int smoothing = s.value("powerSmoothing", 0).toInt();
        const int comboIndex = m_powerSmoothingCombo->findData(smoothing);
        if (comboIndex >= 0)
            m_powerSmoothingCombo->setCurrentIndex(comboIndex);
    }
    if (m_autoSmoothingCheck)
    {
        const bool autoSmoothing = s.value("autoSmoothing", false).toBool();
        m_autoSmoothingCheck->setChecked(autoSmoothing);
        if (m_powerSmoothingCombo)
            m_powerSmoothingCombo->setEnabled(!autoSmoothing);
    }
    if (m_followIntervalSelectionCheck)
    {
        m_followIntervalSelectionCheck->setChecked(
            s.value("followIntervalSelection", true).toBool());
    }
    if (m_mapStyleCombo)
    {
        const int styleValue = s.value("mapStyle", static_cast<int>(MapStyle::Light)).toInt();
        const int comboIndex = m_mapStyleCombo->findData(styleValue);
        if (comboIndex >= 0)
            m_mapStyleCombo->setCurrentIndex(comboIndex);
    }
    if (m_mapAutoContrastCheck)
    {
        m_mapAutoContrastCheck->setChecked(s.value("mapAutoRouteContrast", true).toBool());
    }

    if (m_chartPresetCombo)
    {
        const int presetValue = s.value("chartPreset", kChartPresetCustom).toInt();
        const int comboIndex = m_chartPresetCombo->findData(presetValue);
        if (comboIndex >= 0)
            m_chartPresetCombo->setCurrentIndex(comboIndex);
    }

    if (m_colorMetricCombo)
    {
        const int colorMetricValue = s.value("colorMetric", static_cast<int>(ColorMetric::Power)).toInt();
        const int comboIndex = m_colorMetricCombo->findData(colorMetricValue);
        if (comboIndex >= 0)
            m_colorMetricCombo->setCurrentIndex(comboIndex);
    }

    if (s.contains("activeTab"))
        m_tabWidget->setCurrentIndex(s.value("activeTab", 0).toInt());
    if (m_analysisTabWidget && s.contains("analysisTab"))
        m_analysisTabWidget->setCurrentIndex(s.value("analysisTab", 0).toInt());
}

void MainWindow::addRecentDatabase(const QString& path)
{
    QSettings s("Fitlyzer", "FitlyzerC");
    QStringList recent = s.value("recentDatabases").toStringList();
    recent.removeAll(path);
    recent.prepend(path);
    while (recent.size() > kMaxRecentDatabases)
        recent.removeLast();
    s.setValue("recentDatabases", recent);
}

void MainWindow::updateRecentDatabaseMenu()
{
    if (!m_recentDbMenu) return;

    m_recentDbMenu->clear();
    QSettings s("Fitlyzer", "FitlyzerC");
    const QStringList recent = s.value("recentDatabases").toStringList();

    for (const QString& path : recent)
    {
        auto* act = m_recentDbMenu->addAction(QFileInfo(path).fileName());
        act->setData(path);
        act->setStatusTip(path);
        connect(act, &QAction::triggered, this, &MainWindow::openRecentDatabase);
    }

    if (recent.isEmpty())
    {
        auto* emptyAct = m_recentDbMenu->addAction("(none)");
        emptyAct->setEnabled(false);
    }
}

void MainWindow::openDatabasePath(const QString& path)
{
    QString err;
    if (!m_dbManager.open(path, &err))
    {
        QMessageBox::critical(this, "Database Error",
            "Could not open database:\n" + err);
        return;
    }

    m_controller->setDatabaseManager(&m_dbManager);
    QSettings settings("Fitlyzer", "FitlyzerC");
    settings.setValue("lastDatabase", path);
    addRecentDatabase(path);
    updateRecentDatabaseMenu();
    if (m_analysisQueue)
        m_analysisQueue->setDatabasePath(path);
    if (m_importQueue)
        m_importQueue->setDatabasePath(path);

    if (m_activityBrowser)
        m_activityBrowser->refresh(m_currentAthleteId);

    if (m_calendarWidget)
    {
        m_calendarWidget->setDatabaseManager(&m_dbManager);
        m_calendarWidget->setAthleteId(m_currentAthleteId);
    }

    refreshAthleteSelector();
    updateAthleteLabel();
    updateImportAvailability();
    updateStatsLabel();
    updateStatusBarInfo();
    updateWelcomeScreenVisibility();
    statusBar()->showMessage("Database opened.", 2500);
}

void MainWindow::openDatabase()
{
    const QString startDir = Platform::defaultDatabaseDirectory();
    const QString path = QFileDialog::getOpenFileName(
        this, "Open Database", startDir,
        "Fitlyzer Database (*.fitlyzerdb);;All Files (*)");
    if (path.isEmpty()) return;
    openDatabasePath(path);
}

void MainWindow::createDatabase()
{
    CreateDatabaseDialog dlg(Platform::defaultDatabaseDirectory(), this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    const QString path = dlg.databasePath();
    if (path.isEmpty())
        return;

    QString err;
    if (!m_dbManager.create(path, &err))
    {
        QMessageBox::critical(this, "Database Error",
            "Could not create database:\n" + err);
        return;
    }

    m_controller->setDatabaseManager(&m_dbManager);
    QSettings settings("Fitlyzer", "FitlyzerC");
    settings.setValue("lastDatabase", path);
    m_lastOpenDir = QFileInfo(path).absolutePath();

    if (dlg.shouldCreateDefaultAthlete())
    {
        auto db = m_dbManager.database();
        AthleteRepository repo(db);

        Athlete athlete;
        athlete.firstName = dlg.defaultAthleteFirstName();
        athlete.lastName = dlg.defaultAthleteLastName();
        const int athleteId = repo.insertAthlete(athlete);
        if (athleteId > 0)
        {
            m_currentAthleteId = athleteId;
            m_currentAthleteName = repo.getAthlete(athleteId).fullName();
            m_controller->setCurrentAthlete(athleteId);
            settings.setValue("currentAthleteId", athleteId);
        }
    }

    addRecentDatabase(path);
    updateRecentDatabaseMenu();
    refreshAthleteSelector();
    updateAthleteLabel();
    updateImportAvailability();
    updateStatsLabel();
    updateStatusBarInfo();
    updateWelcomeScreenVisibility();

    QMessageBox::information(this, "Database Created",
        "Database created successfully.\n" + path);
}
