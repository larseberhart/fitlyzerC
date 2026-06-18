// SPDX-License-Identifier: GPL-3

#include "MainWindow.h"

#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSqlQuery>
#include <QVBoxLayout>

#include "ActivityBrowser.h"
#include "AthleteListDialog.h"
#include "CalendarWidget.h"
#include "core/settings/AppSettings.h"
#include "maps/MapRenderer.h"
#include "platform/Platform.h"

// ── Settings helpers ─────────────────────────────────────────────────────────

/// Default climb detection values (mirrors ClimbDetector defaults).
namespace ClimbDefaults
{
    static constexpr double kMinLength    = 0.15;
    static constexpr double kMinGain      = 10.0;
    static constexpr double kMinGradient  = 4.0;
    static constexpr double kStartGrad    = 1.5;
    static constexpr double kDipMeters    = 10.0;
    static constexpr double kDipDistance  = 200.0;
    static constexpr double kSmoothing    = 50.0;
}

void MainWindow::manageAthletes()
{
    if (!m_dbManager.isOpen())
    {
        QMessageBox::information(this, "No Database",
            "Please open or create a database first.");
        return;
    }

    auto db = m_dbManager.database();
    AthleteRepository repo(db);
    AthleteListDialog dlg(repo, this);
    if (dlg.exec() == QDialog::Accepted)
    {
        m_currentAthleteId = dlg.selectedAthleteId();
        m_controller->setCurrentAthlete(m_currentAthleteId);
        m_currentAthleteName = repo.getAthlete(m_currentAthleteId).fullName();
        QSettings("Fitlyzer", "FitlyzerC")
            .setValue("currentAthleteId", m_currentAthleteId);
        updateAthleteLabel();
        refreshAthleteSelector();
        if (m_activityBrowser)
            m_activityBrowser->refresh(m_currentAthleteId);
        updateImportAvailability();
        updateStatusBarInfo();
        updateWelcomeScreenVisibility();
    }
}

void MainWindow::openSettingsDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Settings");
    dialog.setModal(true);

    auto* layout = new QVBoxLayout(&dialog);
    auto* generalGroup = new QGroupBox("General", &dialog);
    auto* form = new QFormLayout(generalGroup);

    auto* dateFormatCombo = new QComboBox(generalGroup);
    dateFormatCombo->addItem("DD-MM-YYYY (14-06-2026)", static_cast<int>(DateFormat::DD_MM_YYYY));
    dateFormatCombo->addItem("YYYY-MM-DD (2026-06-14)", static_cast<int>(DateFormat::YYYY_MM_DD));
    dateFormatCombo->addItem("DD.MM.YYYY (14.06.2026)", static_cast<int>(DateFormat::DD_DOT_MM_DOT_YYYY));
    dateFormatCombo->addItem("MM/DD/YYYY (06/14/2026)", static_cast<int>(DateFormat::MM_DD_YYYY));

    const DateFormat currentFormat = AppSettings::instance().dateFormat();
    const int currentIndex = dateFormatCombo->findData(static_cast<int>(currentFormat));
    dateFormatCombo->setCurrentIndex(currentIndex >= 0 ? currentIndex : 0);

    form->addRow("Date Format:", dateFormatCombo);
    layout->addWidget(generalGroup);

    // Maps group
    auto* mapsGroup = new QGroupBox("Maps", &dialog);
    auto* mapsForm = new QFormLayout(mapsGroup);
    auto* tileCacheCombo = new QComboBox(mapsGroup);
    tileCacheCombo->addItem("128 tiles (~32 MB)",  128);
    tileCacheCombo->addItem("256 tiles (~64 MB)",  256);
    tileCacheCombo->addItem("512 tiles (~128 MB)", 512);
    tileCacheCombo->addItem("1024 tiles (~256 MB)", 1024);
    tileCacheCombo->addItem("2048 tiles (~512 MB)", 2048);
    const int currentCacheSize = AppSettings::instance().tileCacheSize();
    const int cacheSizeIndex = tileCacheCombo->findData(currentCacheSize);
    tileCacheCombo->setCurrentIndex(cacheSizeIndex >= 0 ? cacheSizeIndex : 2);
    mapsForm->addRow("Tile Memory Cache:", tileCacheCombo);
    layout->addWidget(mapsGroup);

    // Climb Detection group
    // Parameters are persisted in QSettings and also reflected in the live
    // spinboxes on ClimbsPage.
    auto* climbGroup = new QGroupBox("Climb Detection", &dialog);
    auto* climbForm  = new QFormLayout(climbGroup);

    QSettings climbSettings("Fitlyzer", "FitlyzerC");
    auto makeSpin = [&climbSettings, &climbGroup](
        const QString& key, double defaultVal, double min, double max, int decimals) -> QDoubleSpinBox*
    {
        auto* spin = new QDoubleSpinBox(climbGroup);
        spin->setRange(min, max);
        spin->setDecimals(decimals);
        spin->setValue(climbSettings.value(key, defaultVal).toDouble());
        return spin;
    };

    auto* csMinLen  = makeSpin("climb/minLength",   ClimbDefaults::kMinLength,    0.1, 20.0,   2);
    auto* csMinGain = makeSpin("climb/minGain",     ClimbDefaults::kMinGain,      1.0, 2000.0, 0);
    auto* csMinGrad = makeSpin("climb/minGradient", ClimbDefaults::kMinGradient,  0.5, 20.0,   1);
    auto* csStGrad  = makeSpin("climb/startGrad",   ClimbDefaults::kStartGrad,    0.5, 20.0,   1);
    auto* csDipM    = makeSpin("climb/dipMeters",   ClimbDefaults::kDipMeters,    0.0, 200.0,  1);
    auto* csDipD    = makeSpin("climb/dipDistance", ClimbDefaults::kDipDistance,  10.0,2000.0, 0);
    auto* csSmooth  = makeSpin("climb/smoothing",   ClimbDefaults::kSmoothing,    1.0, 500.0,  0);

    csMinLen->setSuffix(" km");
    csMinGain->setSuffix(" m");
    csMinGrad->setSuffix(" %");
    csStGrad->setSuffix(" %");
    csDipM->setSuffix(" m");
    csDipD->setSuffix(" m");

    climbForm->addRow("Minimum Length:",    csMinLen);
    climbForm->addRow("Minimum Gain:",      csMinGain);
    climbForm->addRow("Average Gradient:",  csMinGrad);
    climbForm->addRow("Start Gradient:",    csStGrad);
    climbForm->addRow("Dip Elevation:",     csDipM);
    climbForm->addRow("Dip Distance:",      csDipD);
    climbForm->addRow("Smoothing:",         csSmooth);

    auto* restoreDefaultsBtn = new QPushButton("Restore Defaults", climbGroup);
    climbForm->addRow("", restoreDefaultsBtn);
    connect(restoreDefaultsBtn, &QPushButton::clicked, this, [=]() {
        csMinLen->setValue(ClimbDefaults::kMinLength);
        csMinGain->setValue(ClimbDefaults::kMinGain);
        csMinGrad->setValue(ClimbDefaults::kMinGradient);
        csStGrad->setValue(ClimbDefaults::kStartGrad);
        csDipM->setValue(ClimbDefaults::kDipMeters);
        csDipD->setValue(ClimbDefaults::kDipDistance);
        csSmooth->setValue(ClimbDefaults::kSmoothing);
    });

    layout->addWidget(climbGroup);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    const DateFormat selectedFormat = static_cast<DateFormat>(dateFormatCombo->currentData().toInt());
    if (selectedFormat == currentFormat)
        return;

    AppSettings::instance().setDateFormat(selectedFormat);

    const int selectedCacheSize = tileCacheCombo->currentData().toInt();
    if (selectedCacheSize != currentCacheSize)
    {
        AppSettings::instance().setTileCacheSize(selectedCacheSize);
        if (m_mapRenderer)
            m_mapRenderer->setTileCacheSize(selectedCacheSize);
    }

    // Apply climb detection settings and update the live spinboxes on ClimbsPage.
    QSettings cs("Fitlyzer", "FitlyzerC");
    cs.setValue("climb/minLength",   csMinLen->value());
    cs.setValue("climb/minGain",     csMinGain->value());
    cs.setValue("climb/minGradient", csMinGrad->value());
    cs.setValue("climb/startGrad",   csStGrad->value());
    cs.setValue("climb/dipMeters",   csDipM->value());
    cs.setValue("climb/dipDistance", csDipD->value());
    cs.setValue("climb/smoothing",   csSmooth->value());

    // Sync the live ClimbsPage spinboxes (they auto-trigger detection on change).
    auto applyIfChanged = [](QDoubleSpinBox* spin, double newVal)
    {
        if (spin && !qFuzzyCompare(spin->value(), newVal))
            spin->setValue(newVal);
    };
    applyIfChanged(m_climbMinLengthSpin,    csMinLen->value());
    applyIfChanged(m_climbMinGainSpin,      csMinGain->value());
    applyIfChanged(m_climbMinGradientSpin,  csMinGrad->value());
    applyIfChanged(m_climbStartGradientSpin, csStGrad->value());
    applyIfChanged(m_climbDipMetersSpin,    csDipM->value());
    applyIfChanged(m_climbDipDistanceSpin,  csDipD->value());
    applyIfChanged(m_climbSmoothingSpin,    csSmooth->value());

    if (selectedFormat == currentFormat)
        return;

    if (m_activityBrowser)
        m_activityBrowser->refresh(m_currentAthleteId);

    updateStatsLabel();
}

void MainWindow::backupDatabase()
{
    if (!m_dbManager.isOpen())
    {
        QMessageBox::information(this, "No Database",
            "No database is open.");
        return;
    }
    const QString startPath = Platform::defaultBackupPath(m_dbManager.path());
    const QString dest = QFileDialog::getSaveFileName(
        this, "Backup Database", startPath,
        "Fitlyzer Database (*.fitlyzerdb)");
    if (dest.isEmpty()) return;

    // Checkpoint the WAL before copying so the backup file is self-contained.
    {
        auto db = m_dbManager.database();
        QSqlQuery q(db);
        q.exec("PRAGMA wal_checkpoint(FULL)");
    }

    if (QFile::exists(dest) && !QFile::remove(dest))
    {
        QMessageBox::critical(this, "Backup Failed",
            "Cannot overwrite existing file:\n" + dest);
        return;
    }

    if (QFile::copy(m_dbManager.path(), dest))
        QMessageBox::information(this, "Backup", "Backup saved to:\n" + dest);
    else
        QMessageBox::critical(this, "Backup Failed",
            "Could not copy database to:\n" + dest);
}

void MainWindow::updateAthleteLabel()
{
    if (m_athleteLabelAct)
        m_athleteLabelAct->setText(
            m_currentAthleteName.isEmpty()
                ? "(no athlete selected)"
                : "Current: " + m_currentAthleteName);
}

void MainWindow::openRecentDatabase()
{
    auto* action = qobject_cast<QAction*>(sender());
    if (!action) return;
    const QString path = action->data().toString();
    if (path.isEmpty()) return;
    openDatabasePath(path);
}

void MainWindow::onAthleteSelectionChanged(int index)
{
    if (!m_athleteCombo || index < 0)
        return;

    const int athleteId = m_athleteCombo->currentData().toInt();
    if (athleteId <= 0)
    {
        m_currentAthleteId = -1;
        m_currentAthleteName.clear();
    }
    else
    {
        m_currentAthleteId = athleteId;
        m_currentAthleteName = m_athleteCombo->currentText();
        m_controller->setCurrentAthlete(athleteId);
    }

    QSettings("Fitlyzer", "FitlyzerC").setValue("currentAthleteId", m_currentAthleteId);
    updateAthleteLabel();
    if (m_activityBrowser)
        m_activityBrowser->refresh(m_currentAthleteId);
    if (m_calendarWidget)
        m_calendarWidget->setAthleteId(m_currentAthleteId);
    updateChartAnalysisEmptyStates();
    updateImportAvailability();
    updateStatsLabel();
    updateStatusBarInfo();
    updateWelcomeScreenVisibility();
}
