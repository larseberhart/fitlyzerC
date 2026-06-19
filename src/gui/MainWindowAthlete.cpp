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
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSqlQuery>
#include <QTabWidget>
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
    // ── Dialog frame ──────────────────────────────────────────────────────
    QDialog dialog(this);
    dialog.setWindowTitle("Settings");
    dialog.setModal(true);
    dialog.resize(540, 480);

    auto* dlgLayout = new QVBoxLayout(&dialog);
    dlgLayout->setContentsMargins(12, 12, 12, 12);
    dlgLayout->setSpacing(8);

    auto* tabs = new QTabWidget(&dialog);

    // ── Tab: General ─────────────────────────────────────────────────────
    {
        auto* page = new QWidget(tabs);
        auto* vl   = new QVBoxLayout(page);
        vl->setContentsMargins(12, 12, 12, 12);

        auto* generalGroup = new QGroupBox("General", page);
        auto* form = new QFormLayout(generalGroup);

        auto* dateFormatCombo = new QComboBox(generalGroup);
        dateFormatCombo->addItem("DD-MM-YYYY (14-06-2026)", static_cast<int>(DateFormat::DD_MM_YYYY));
        dateFormatCombo->addItem("YYYY-MM-DD (2026-06-14)", static_cast<int>(DateFormat::YYYY_MM_DD));
        dateFormatCombo->addItem("DD.MM.YYYY (14.06.2026)", static_cast<int>(DateFormat::DD_DOT_MM_DOT_YYYY));
        dateFormatCombo->addItem("MM/DD/YYYY (06/14/2026)", static_cast<int>(DateFormat::MM_DD_YYYY));
        const DateFormat currentFormat = AppSettings::instance().dateFormat();
        dateFormatCombo->setCurrentIndex(
            dateFormatCombo->findData(static_cast<int>(currentFormat)));
        form->addRow("Date Format:", dateFormatCombo);

        vl->addWidget(generalGroup);
        vl->addStretch(1);
        tabs->addTab(page, "General");

        // Store pointer for accept handler
        dialog.setProperty("dateFormatCombo", QVariant::fromValue(static_cast<QObject*>(dateFormatCombo)));
    }

    // ── Tab: Athletes ────────────────────────────────────────────────────
    {
        auto* page = new QWidget(tabs);
        auto* vl   = new QVBoxLayout(page);
        vl->setContentsMargins(12, 12, 12, 12);

        auto* lbl = new QLabel(
            "Athlete profile defaults and team settings are managed in this section.\n"
            "Use the main Athletes manager for profile CRUD.",
            page);
        lbl->setWordWrap(true);
        lbl->setStyleSheet("color: #64748b;");
        vl->addWidget(lbl);
        vl->addStretch(1);

        tabs->addTab(page, "Athletes");
    }

    // ── Tab: Import ──────────────────────────────────────────────────────
    {
        auto* page = new QWidget(tabs);
        auto* vl   = new QVBoxLayout(page);
        vl->setContentsMargins(12, 12, 12, 12);

        auto* lbl = new QLabel(
            "Import defaults (auto-tagging, duplicate handling, file watchers)\n"
            "will be consolidated here.",
            page);
        lbl->setWordWrap(true);
        lbl->setStyleSheet("color: #64748b;");
        vl->addWidget(lbl);
        vl->addStretch(1);

        tabs->addTab(page, "Import");
    }

    // ── Tab: Maps ────────────────────────────────────────────────────────
    {
        auto* page = new QWidget(tabs);
        auto* vl   = new QVBoxLayout(page);
        vl->setContentsMargins(12, 12, 12, 12);

        auto* mapsGroup = new QGroupBox("Maps", page);
        auto* mapsForm  = new QFormLayout(mapsGroup);

        auto* tileCacheCombo = new QComboBox(mapsGroup);
        tileCacheCombo->addItem("128 tiles (~32 MB)",   128);
        tileCacheCombo->addItem("256 tiles (~64 MB)",   256);
        tileCacheCombo->addItem("512 tiles (~128 MB)",  512);
        tileCacheCombo->addItem("1024 tiles (~256 MB)", 1024);
        tileCacheCombo->addItem("2048 tiles (~512 MB)", 2048);
        const int currentCacheSize = AppSettings::instance().tileCacheSize();
        tileCacheCombo->setCurrentIndex(
            tileCacheCombo->findData(currentCacheSize) >= 0
            ? tileCacheCombo->findData(currentCacheSize) : 2);
        mapsForm->addRow("Tile Memory Cache:", tileCacheCombo);

        vl->addWidget(mapsGroup);
        vl->addStretch(1);
        tabs->addTab(page, "Maps");

        dialog.setProperty("tileCacheCombo", QVariant::fromValue(static_cast<QObject*>(tileCacheCombo)));
    }

    // ── Tab: Climb Detection ─────────────────────────────────────────────
    QDoubleSpinBox *csMinLen, *csMinGain, *csMinGrad, *csStGrad,
                   *csDipM, *csDipD, *csSmooth;
    {
        auto* page = new QWidget(tabs);
        auto* vl   = new QVBoxLayout(page);
        vl->setContentsMargins(12, 12, 12, 12);

        auto* climbGroup = new QGroupBox("Climb Detection Parameters", page);
        auto* climbForm  = new QFormLayout(climbGroup);

        QSettings cs("Fitlyzer", "FitlyzerC");
        auto makeSpin = [&cs, climbGroup](
            const QString& key, double defaultVal, double min, double max, int decimals,
            const QString& suffix) -> QDoubleSpinBox*
        {
            auto* spin = new QDoubleSpinBox(climbGroup);
            spin->setRange(min, max);
            spin->setDecimals(decimals);
            spin->setValue(cs.value(key, defaultVal).toDouble());
            if (!suffix.isEmpty()) spin->setSuffix(suffix);
            return spin;
        };

        csMinLen  = makeSpin("climb/minLength",   ClimbDefaults::kMinLength,   0.1,  20.0,   2, " km");
        csMinGain = makeSpin("climb/minGain",     ClimbDefaults::kMinGain,     1.0,  2000.0, 0, " m");
        csMinGrad = makeSpin("climb/minGradient", ClimbDefaults::kMinGradient, 0.5,  20.0,   1, " %");
        csStGrad  = makeSpin("climb/startGrad",   ClimbDefaults::kStartGrad,   0.5,  20.0,   1, " %");
        csDipM    = makeSpin("climb/dipMeters",   ClimbDefaults::kDipMeters,   0.0,  200.0,  1, " m");
        csDipD    = makeSpin("climb/dipDistance", ClimbDefaults::kDipDistance, 10.0, 2000.0, 0, " m");
        csSmooth  = makeSpin("climb/smoothing",   ClimbDefaults::kSmoothing,   1.0,  500.0,  0, "");

        climbForm->addRow("Minimum Length:",   csMinLen);
        climbForm->addRow("Minimum Gain:",     csMinGain);
        climbForm->addRow("Avg Gradient:",     csMinGrad);
        climbForm->addRow("Start Gradient:",   csStGrad);
        climbForm->addRow("Dip Elevation:",    csDipM);
        climbForm->addRow("Dip Distance:",     csDipD);
        climbForm->addRow("Smoothing:",        csSmooth);

        auto* restoreBtn = new QPushButton("Restore Defaults", climbGroup);
        climbForm->addRow("", restoreBtn);
        connect(restoreBtn, &QPushButton::clicked, this, [=]() {
            csMinLen->setValue(ClimbDefaults::kMinLength);
            csMinGain->setValue(ClimbDefaults::kMinGain);
            csMinGrad->setValue(ClimbDefaults::kMinGradient);
            csStGrad->setValue(ClimbDefaults::kStartGrad);
            csDipM->setValue(ClimbDefaults::kDipMeters);
            csDipD->setValue(ClimbDefaults::kDipDistance);
            csSmooth->setValue(ClimbDefaults::kSmoothing);
        });

        vl->addWidget(climbGroup);
        vl->addStretch(1);
        tabs->addTab(page, "Climb Detection");
    }

    // ── Tab: Analysis (stub) ─────────────────────────────────────────────
    {
        auto* page  = new QWidget(tabs);
        auto* vl    = new QVBoxLayout(page);
        vl->setContentsMargins(12, 12, 12, 12);
        auto* lbl   = new QLabel("Additional analysis preferences — coming soon.", page);
        lbl->setStyleSheet("color: #64748b;");
        lbl->setWordWrap(true);
        vl->addWidget(lbl);
        vl->addStretch(1);
        tabs->addTab(page, "Analysis");
    }

    // ── Tab: Charts (stub) ───────────────────────────────────────────────
    {
        auto* page  = new QWidget(tabs);
        auto* vl    = new QVBoxLayout(page);
        vl->setContentsMargins(12, 12, 12, 12);
        auto* lbl   = new QLabel("Chart defaults and presets configuration — coming soon.", page);
        lbl->setStyleSheet("color: #64748b;");
        lbl->setWordWrap(true);
        vl->addWidget(lbl);
        vl->addStretch(1);
        tabs->addTab(page, "Charts");
    }

    // ── Tab: Video (stub) ────────────────────────────────────────────────
    {
        auto* page = new QWidget(tabs);
        auto* vl   = new QVBoxLayout(page);
        vl->setContentsMargins(12, 12, 12, 12);
        auto* lbl  = new QLabel("Video export defaults — coming soon.", page);
        lbl->setStyleSheet("color: #64748b;");
        lbl->setWordWrap(true);
        vl->addWidget(lbl);
        vl->addStretch(1);
        tabs->addTab(page, "Video");
    }

    // ── Tab: Advanced (stub) ─────────────────────────────────────────────
    {
        auto* page = new QWidget(tabs);
        auto* vl   = new QVBoxLayout(page);
        vl->setContentsMargins(12, 12, 12, 12);
        auto* lbl  = new QLabel("Advanced diagnostics and storage options — coming soon.", page);
        lbl->setStyleSheet("color: #64748b;");
        lbl->setWordWrap(true);
        vl->addWidget(lbl);
        vl->addStretch(1);
        tabs->addTab(page, "Advanced");
    }

    dlgLayout->addWidget(tabs, 1);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    dlgLayout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    // Retrieve current values before we possibly changed them via combo boxes.
    auto* dfCombo = qobject_cast<QComboBox*>(
        dialog.property("dateFormatCombo").value<QObject*>());
    auto* tcCombo = qobject_cast<QComboBox*>(
        dialog.property("tileCacheCombo").value<QObject*>());

    // General: date format
    const DateFormat previousFormat = AppSettings::instance().dateFormat();
    if (dfCombo)
    {
        const DateFormat selectedFormat =
            static_cast<DateFormat>(dfCombo->currentData().toInt());
        if (selectedFormat != previousFormat)
        {
            AppSettings::instance().setDateFormat(selectedFormat);
            if (m_activityBrowser)
                m_activityBrowser->refresh(m_currentAthleteId);
            updateStatsLabel();
        }
    }

    // Maps: tile cache size
    if (tcCombo)
    {
        const int selectedCacheSize = tcCombo->currentData().toInt();
        const int prevCacheSize = AppSettings::instance().tileCacheSize();
        if (selectedCacheSize != prevCacheSize)
        {
            AppSettings::instance().setTileCacheSize(selectedCacheSize);
            if (m_mapRenderer)
                m_mapRenderer->setTileCacheSize(selectedCacheSize);
        }
    }

    // Climb detection: persist and sync live spinboxes.
    QSettings cs("Fitlyzer", "FitlyzerC");
    cs.setValue("climb/minLength",   csMinLen->value());
    cs.setValue("climb/minGain",     csMinGain->value());
    cs.setValue("climb/minGradient", csMinGrad->value());
    cs.setValue("climb/startGrad",   csStGrad->value());
    cs.setValue("climb/dipMeters",   csDipM->value());
    cs.setValue("climb/dipDistance", csDipD->value());
    cs.setValue("climb/smoothing",   csSmooth->value());

    auto applyIfChanged = [](QDoubleSpinBox* spin, double newVal)
    {
        if (spin && !qFuzzyCompare(spin->value(), newVal))
            spin->setValue(newVal);
    };
    applyIfChanged(m_climbMinLengthSpin,     csMinLen->value());
    applyIfChanged(m_climbMinGainSpin,       csMinGain->value());
    applyIfChanged(m_climbMinGradientSpin,   csMinGrad->value());
    applyIfChanged(m_climbStartGradientSpin, csStGrad->value());
    applyIfChanged(m_climbDipMetersSpin,     csDipM->value());
    applyIfChanged(m_climbDipDistanceSpin,   csDipD->value());
    applyIfChanged(m_climbSmoothingSpin,     csSmooth->value());
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
