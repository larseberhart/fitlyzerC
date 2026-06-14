#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QBrush>
#include <QCheckBox>
#include <QCloseEvent>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDate>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QFileSystemWatcher>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QKeySequence>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStandardPaths>
#include <QSplitter>
#include <QSqlQuery>
#include <QStatusBar>
#include <QStandardPaths>
#include <QStyle>
#include <QTableWidgetItem>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include "ActivityBrowser.h"
#include "AboutDialog.h"
#include "AthleteHeaderWidget.h"
#include "AthleteDialog.h"
#include "AthleteListDialog.h"
#include "CalendarWidget.h"
#include "CreateDatabaseDialog.h"
#include "EditActivityDialog.h"
#include "IntervalEditorDialog.h"
#include "WelcomeWidget.h"
#include "charts/FitnessChartWidget.h"
#include "charts/PowerCurveWidget.h"
#include "charts/PowerHistogramWidget.h"
#include "charts/RideChartWidget.h"
#include "core/settings/AppSettings.h"
#include "core/settings/DateFormatter.h"
#include "core/zones/ZoneCalculator.h"
#include "database/AthleteRepository.h"
#include "database/IntervalRepository.h"
#include "maps/MapRenderer.h"
#include "model/RideDataSerializer.h"
#include "platform/Platform.h"
#include "video/VideoExportDialog.h"
#include "video/VideoExportSettings.h"

#include <cfloat>
#include <cmath>
#include <map>
#include <numeric>

// ── Helpers ─────────────────────────────────────────────────────────────────

static QString fmtDur(double seconds)
{
    const int total = static_cast<int>(seconds);
    const int h = total / 3600;
    const int m = (total % 3600) / 60;
    const int s = total % 60;
    return h > 0
        ? QString("%1:%2:%3")
              .arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'))
        : QString("%1:%2")
              .arg(m).arg(s, 2, 10, QChar('0'));
}

static QString formatActivityDateLabel(const QString& isoDate)
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

static QDate activityDateForFtpContext(const Activity& activity)
{
    const QString rawDate = !activity.startTime.isEmpty()
        ? activity.startTime.left(10)
        : activity.importedAt.left(10);
    return QDate::fromString(rawDate, Qt::ISODate);
}

static constexpr int kMaxRecentDatabases = 8;

enum ChartPresetId
{
    ChartPresetCustom = 0,
    ChartPresetPowerAnalysis,
    ChartPresetHeartRateFocus,
    ChartPresetRaceReview
};

static constexpr int kIntervalStartRole = Qt::UserRole + 1;
static constexpr int kIntervalEndRole = Qt::UserRole + 2;

static QString rangeLabelForZone(const Zone& zone, ColorMetric metric)
{
    const QString unit = colorMetricUnit(metric);
    const QString minText = QString::number(zone.minValue, 'f', metric == ColorMetric::Speed ? 1 : 0);
    const QString maxText = QString::number(zone.maxValue, 'f', metric == ColorMetric::Speed ? 1 : 0);

    if (zone.maxValue >= DBL_MAX / 2.0)
        return QString("> %1 %2").arg(minText, unit).trimmed();

    return QString("%1 - %2 %3").arg(minText, maxText, unit).trimmed();
}

static QString sanitizeVideoFilePart(QString text)
{
    static const QString invalid = QStringLiteral("\\/:*?\"<>|");
    for (const QChar c : invalid)
        text.replace(c, '-');
    text = text.simplified().replace(' ', '-');
    while (text.contains("--"))
        text.replace("--", "-");
    return text.trimmed();
}

static QString resolveFfmpegExecutablePath()
{
    QStringList candidates;

#ifdef FITLYZER_FFMPEG_PATH
    const QString configured = QString::fromUtf8(FITLYZER_FFMPEG_PATH);
    if (!configured.isEmpty())
    {
        candidates << configured;
        candidates << QCoreApplication::applicationDirPath() + "/" + configured;
        candidates << QDir(QCoreApplication::applicationDirPath()).filePath("../Resources/" + configured);
    }
#endif

    candidates << QStandardPaths::findExecutable("ffmpeg");

    for (const QString& candidate : candidates)
    {
        if (candidate.isEmpty())
            continue;
        const QFileInfo fi(candidate);
        if (fi.exists() && fi.isFile())
            return fi.absoluteFilePath();
    }

    return QStringLiteral("ffmpeg");
}

// ── Constructor ─────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setAcceptDrops(true);

    m_controller = new WorkoutController(this);
    m_controller->setDatabaseManager(&m_dbManager);

    buildUI();

    connect(m_controller, &WorkoutController::workoutLoaded,
            this, &MainWindow::onWorkoutLoaded);
    connect(m_controller, &WorkoutController::activityImported,
            this, &MainWindow::onActivityImported);

    if (m_activityBrowser)
    {
        connect(m_activityBrowser, &ActivityBrowser::fitFilesDropped,
                this, &MainWindow::importFiles);
    }

    setupShortcuts();
    loadSettings();
    applyChartHeight();

    QSettings s("Fitlyzer", "FitlyzerC");
    if (!s.contains("firstLaunchCompleted"))
        s.setValue("firstLaunchCompleted", false);
    m_firstLaunchCompleted = s.value("firstLaunchCompleted", false).toBool();

    // Re-open last database and athlete
    const QString lastDb = s.value("lastDatabase").toString();
    if (!lastDb.isEmpty())
    {
        QString err;
        if (!m_dbManager.open(lastDb, &err))
        {
            QMessageBox::warning(this, "Database",
                QString("Could not open last database:\n%1\n\n%2")
                    .arg(lastDb).arg(err));
        }
        else
        {
            m_controller->setDatabaseManager(&m_dbManager);
            const int aid = s.value("currentAthleteId", -1).toInt();
            if (aid > 0)
            {
                m_currentAthleteId = aid;
                m_controller->setCurrentAthlete(aid);
                auto db = m_dbManager.database();
                AthleteRepository repo(db);
                m_currentAthleteName = repo.getAthlete(aid).fullName();
            }
            updateAthleteLabel();
                refreshAthleteSelector();
            if (m_activityBrowser)
                m_activityBrowser->refresh(m_currentAthleteId);
        }
    }

    updateRecentDatabaseMenu();
    refreshAthleteSelector();
    updateImportAvailability();
    updateStatsLabel();
    updateStatusBarInfo();
    updateWelcomeScreenVisibility();
    configureFolderWatcher();
}

// ── Close event ──────────────────────────────────────────────────────────────

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

// ── UI Construction ─────────────────────────────────────────────────────────

void MainWindow::buildUI()
{
    // -- Menu bar ----------------------------------------------------------
    {
        // File menu (database-oriented workflow)
        auto* fileMenu = menuBar()->addMenu("&File");

        auto* openDbAct = new QAction("Open Database...", this);
        connect(openDbAct, &QAction::triggered, this, &MainWindow::openDatabase);
        fileMenu->addAction(openDbAct);

        auto* newDbAct = new QAction("Create Database...", this);
        connect(newDbAct, &QAction::triggered, this, &MainWindow::createDatabase);
        fileMenu->addAction(newDbAct);

        auto* backupAct = new QAction("Backup Database...", this);
        connect(backupAct, &QAction::triggered, this, &MainWindow::backupDatabase);
        fileMenu->addAction(backupAct);

        fileMenu->addSeparator();
        m_recentDbMenu = fileMenu->addMenu("Recent Databases");

        fileMenu->addSeparator();

        auto* exitAct = new QAction("E&xit", this);
        exitAct->setShortcut(QKeySequence::Quit);
        connect(exitAct, &QAction::triggered, qApp, &QApplication::quit);
        fileMenu->addAction(exitAct);

        // Activities menu
        auto* actsMenu = menuBar()->addMenu("&Activities");

        m_importAct = new QAction("Import Activities...", this);
        m_importAct->setShortcut(QKeySequence::Open);
        m_importAct->setEnabled(false);
        connect(m_importAct, &QAction::triggered, this, &MainWindow::importActivities);
        actsMenu->addAction(m_importAct);

        auto* createVideoAct = new QAction("Create Video...", this);
        connect(createVideoAct, &QAction::triggered, this, &MainWindow::createVideo);
        actsMenu->addAction(createVideoAct);

        // Athletes menu
        m_athletesMenu = menuBar()->addMenu("&Athletes");

        auto* manageAct = new QAction("Manage Athletes...", this);
        connect(manageAct, &QAction::triggered, this, &MainWindow::manageAthletes);
        m_athletesMenu->addAction(manageAct);

        // Tools menu for power-user actions
        auto* toolsMenu = menuBar()->addMenu("&Tools");
        m_previewAct = new QAction("Preview FIT File...", this);
        connect(m_previewAct, &QAction::triggered, this, &MainWindow::previewFitFile);
        toolsMenu->addAction(m_previewAct);
        toolsMenu->addSeparator();
        auto* settingsAct = new QAction("Settings...", this);
        settingsAct->setShortcut(QKeySequence::Preferences);
        connect(settingsAct, &QAction::triggered, this, &MainWindow::openSettingsDialog);
        toolsMenu->addAction(settingsAct);

        // View and Help placeholders for expected desktop structure
        auto* viewMenu = menuBar()->addMenu("&View");
        viewMenu->addAction("Reset Zoom", QKeySequence("Ctrl+0"), this, &MainWindow::resetAllZoom);
        viewMenu->addAction("Increase Chart Height", this, &MainWindow::increaseChartHeight);
        viewMenu->addAction("Decrease Chart Height", this, &MainWindow::decreaseChartHeight);
        auto* helpMenu = menuBar()->addMenu("&Help");
        helpMenu->addAction("&About", this, &MainWindow::showAbout);

    }

    // ── Central widget ────────────────────────────────────────────────────
    auto* central    = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(6);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // ── Header panel ──────────────────────────────────────────────────────
    m_athleteHeader = new AthleteHeaderWidget(this);
    mainLayout->addWidget(m_athleteHeader);

    m_welcomeWidget = new WelcomeWidget(this);
    mainLayout->addWidget(m_welcomeWidget, 1);
    m_welcomeWidget->setVisible(false);
    connect(m_welcomeWidget, &WelcomeWidget::importRequested, this, &MainWindow::importActivities);
    connect(m_welcomeWidget, &WelcomeWidget::openDatabaseRequested, this, &MainWindow::openDatabase);
    connect(m_welcomeWidget, &WelcomeWidget::createDatabaseRequested, this, &MainWindow::createDatabase);

    // ── Tab widget ────────────────────────────────────────────────────────
    m_tabWidget = new QTabWidget;
    m_analysisTabWidget = new QTabWidget;

    auto* analysisContainer = new QWidget(this);
    auto* analysisLayout = new QVBoxLayout(analysisContainer);
    analysisLayout->setContentsMargins(0, 0, 0, 0);
    analysisLayout->setSpacing(6);

    auto* colorBar = new QHBoxLayout;
    colorBar->setContentsMargins(0, 0, 0, 0);
    colorBar->addWidget(new QLabel("Color By:"));

    m_colorMetricCombo = new QComboBox(this);
    auto addColorMetric = [this](ColorMetric metric)
    {
        m_colorMetricCombo->addItem(colorMetricDisplayName(metric), static_cast<int>(metric));
    };
    addColorMetric(ColorMetric::None);
    addColorMetric(ColorMetric::Power);
    addColorMetric(ColorMetric::HeartRate);
    addColorMetric(ColorMetric::Cadence);
    addColorMetric(ColorMetric::Speed);
    addColorMetric(ColorMetric::Altitude);
    m_colorMetricCombo->setCurrentIndex(
        m_colorMetricCombo->findData(static_cast<int>(ColorMetric::Power)));
    colorBar->addWidget(m_colorMetricCombo);
    colorBar->addStretch(1);
    analysisLayout->addLayout(colorBar);

    // == Tab 0: Activities Browser ==
    {
        m_activityBrowser = new ActivityBrowser(&m_dbManager, this);
        m_tabWidget->addTab(m_activityBrowser, "Activities");

        connect(m_activityBrowser, &ActivityBrowser::activitySelected,
                this, [this](int activityId)
        {
            QString err;
            if (!m_controller->loadActivity(activityId, err))
                QMessageBox::critical(this, "Load Error", err);
            else
            {
                m_tabWidget->setCurrentIndex(kTabAnalysis);
                if (m_analysisTabWidget)
                    m_analysisTabWidget->setCurrentIndex(kAnalysisTabCharts);
            }
        });

        connect(m_activityBrowser, &ActivityBrowser::activityDeleted,
                this, [this](int activityId)
        {
            // If the currently loaded activity was deleted, clear the view.
            (void)activityId;
            updateStatsLabel();
            updateStatusBarInfo();
            updateWelcomeScreenVisibility();
        });
    }

    // == Tab 1: Charts ==
    {
        // -- Metrics dropdown ------------------------------------------------
        m_showPower    = new QCheckBox("Power");       m_showPower->setChecked(true);
        m_showHR       = new QCheckBox("Heart Rate");  m_showHR->setChecked(true);
        m_showCadence  = new QCheckBox("Cadence");     m_showCadence->setChecked(true);
        m_showSpeed    = new QCheckBox("Speed");       m_showSpeed->setChecked(true);
        m_showAltitude = new QCheckBox("Altitude");    m_showAltitude->setChecked(true);

        auto* metricsBtn = new QToolButton;
        metricsBtn->setText("Metrics");
        metricsBtn->setPopupMode(QToolButton::InstantPopup);
        auto* metricsMenu = new QMenu(metricsBtn);
        auto addMetricAction = [metricsMenu](const QString& title, QCheckBox* box)
        {
            auto* act = metricsMenu->addAction(title);
            act->setCheckable(true);
            act->setChecked(box->isChecked());
            QObject::connect(act, &QAction::toggled, box, &QCheckBox::setChecked);
            QObject::connect(box, &QCheckBox::toggled, act, &QAction::setChecked);
        };
        addMetricAction("Power", m_showPower);
        addMetricAction("Heart Rate", m_showHR);
        addMetricAction("Cadence", m_showCadence);
        addMetricAction("Speed", m_showSpeed);
        addMetricAction("Altitude", m_showAltitude);
        metricsBtn->setMenu(metricsMenu);

        m_powerSmoothingCombo = new QComboBox(this);
        m_powerSmoothingCombo->addItem("Raw", 0);
        m_powerSmoothingCombo->addItem("3 sec", 3);
        m_powerSmoothingCombo->addItem("5 sec", 5);
        m_powerSmoothingCombo->addItem("10 sec", 10);
        m_powerSmoothingCombo->addItem("30 sec", 30);
        m_powerSmoothingCombo->addItem("60 sec", 60);

        m_autoSmoothingCheck = new QCheckBox("Auto Smoothing", this);
        m_autoSmoothingCheck->setChecked(false);

        // -- Chart height / fit controls -------------------------------------
        m_fitChartsButton           = new QPushButton("Reset Zoom");
        m_chartHeightIncreaseButton = new QPushButton("Increase Height");
        m_chartHeightDecreaseButton = new QPushButton("Decrease Height");
        m_useEstimatedFtpButton     = new QPushButton("Use Estimated FTP");
        auto* editActivityButton    = new QPushButton("Activity Properties");
        m_fitChartsButton->setFixedWidth(110);
        m_chartHeightIncreaseButton->setFixedWidth(130);
        m_chartHeightDecreaseButton->setFixedWidth(130);
        m_useEstimatedFtpButton->setFixedWidth(150);
        editActivityButton->setFixedWidth(150);
        m_fitChartsButton->setToolTip("Reset all chart zoom to full ride (Ctrl+0)");
        m_chartHeightIncreaseButton->setToolTip("Increase chart height");
        m_chartHeightDecreaseButton->setToolTip("Decrease chart height");
        m_useEstimatedFtpButton->setToolTip("Estimate FTP as 95% of best 20-minute power");
        m_useEstimatedFtpButton->setEnabled(false);

        connect(m_fitChartsButton, &QPushButton::clicked,
                this, &MainWindow::resetAllZoom);
        connect(m_chartHeightIncreaseButton, &QPushButton::clicked,
                this, &MainWindow::increaseChartHeight);
        connect(m_chartHeightDecreaseButton, &QPushButton::clicked,
                this, &MainWindow::decreaseChartHeight);
        connect(editActivityButton, &QPushButton::clicked,
            this, &MainWindow::editCurrentActivityProperties);
        connect(m_useEstimatedFtpButton, &QPushButton::clicked,
            this, &MainWindow::applyEstimatedFtpForCurrentAthlete);

        auto* ctrlBar = new QHBoxLayout;
        ctrlBar->setContentsMargins(0, 0, 0, 0);
        ctrlBar->addWidget(metricsBtn);
        ctrlBar->addSpacing(10);
        ctrlBar->addWidget(new QLabel("Preset:"));

        m_chartPresetCombo = new QComboBox(this);
        m_chartPresetCombo->addItem("Custom", ChartPresetCustom);
        m_chartPresetCombo->addItem("Power Analysis", ChartPresetPowerAnalysis);
        m_chartPresetCombo->addItem("Heart Rate Focus", ChartPresetHeartRateFocus);
        m_chartPresetCombo->addItem("Race Review", ChartPresetRaceReview);
        m_chartPresetCombo->setCurrentIndex(0);
        ctrlBar->addWidget(m_chartPresetCombo);

        ctrlBar->addSpacing(10);
        ctrlBar->addWidget(new QLabel("Power Smoothing:"));
        ctrlBar->addWidget(m_powerSmoothingCombo);
        ctrlBar->addWidget(m_autoSmoothingCheck);
        ctrlBar->addStretch(1);
        ctrlBar->addSpacing(8);
        ctrlBar->addWidget(m_fitChartsButton);
        ctrlBar->addWidget(m_chartHeightIncreaseButton);
        ctrlBar->addWidget(m_chartHeightDecreaseButton);
        ctrlBar->addWidget(m_useEstimatedFtpButton);
        ctrlBar->addWidget(editActivityButton);

        // -- Charts ----------------------------------------------------------
        m_powerChart    = new RideChartWidget(Metric::Power);
        m_hrChart       = new RideChartWidget(Metric::HeartRate);
        m_cadenceChart  = new RideChartWidget(Metric::Cadence);
        m_speedChart    = new RideChartWidget(Metric::Speed);
        m_altitudeChart = new RideChartWidget(Metric::Altitude);

        connect(m_powerSmoothingCombo,
                qOverload<int>(&QComboBox::currentIndexChanged),
                this,
                [this](int)
        {
            if (!m_powerSmoothingCombo)
                return;
            applyChartSmoothing();
        });

        connect(m_autoSmoothingCheck, &QCheckBox::toggled,
                this, [this](bool checked)
        {
            if (!m_powerSmoothingCombo)
                return;
            m_powerSmoothingCombo->setEnabled(!checked);
            applyChartSmoothing();
        });

        connect(m_chartPresetCombo,
                qOverload<int>(&QComboBox::currentIndexChanged),
                this,
                [this](int)
        {
            if (!m_chartPresetCombo)
                return;
            applyChartPreset(m_chartPresetCombo->currentData().toInt());
        });

        // Container inside scroll area.
        // setWidgetResizable(true) fills the viewport width automatically.
        // We set a minimumHeight so the vertical scrollbar appears when the
        // window is shorter than the total chart area.
        m_chartsContainer = new QWidget;
        m_chartsLayout    = new QVBoxLayout(m_chartsContainer);
        m_chartsLayout->setSpacing(4);
        m_chartsLayout->setContentsMargins(0, 0, 0, 0);
        m_chartsLayout->addWidget(m_powerChart);
        m_chartsLayout->addWidget(m_hrChart);
        m_chartsLayout->addWidget(m_cadenceChart);
        m_chartsLayout->addWidget(m_speedChart);
        m_chartsLayout->addWidget(m_altitudeChart);
        m_chartsLayout->addStretch();

        m_chartScroll = new QScrollArea;
        m_chartScroll->setWidget(m_chartsContainer);
        m_chartScroll->setWidgetResizable(true);
        m_chartScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_chartScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        m_chartScroll->viewport()->installEventFilter(this);

        // Tab container: controls bar + scroll area
        auto* chartsTab = new QWidget;
        auto* chartsVL  = new QVBoxLayout(chartsTab);
        chartsVL->setContentsMargins(0, 0, 0, 0);
        chartsVL->setSpacing(4);
        chartsVL->addLayout(ctrlBar);

        m_colorLegendWidget = new QWidget(chartsTab);
        m_colorLegendLayout = new QHBoxLayout(m_colorLegendWidget);
        m_colorLegendLayout->setContentsMargins(0, 0, 0, 0);
        m_colorLegendLayout->setSpacing(8);
        chartsVL->addWidget(m_colorLegendWidget);
        // Integrated analysis workspace: charts + map + intervals/laps/notes.
        m_mapRenderer = new MapRenderer;

        auto* mapPanel = new QWidget(chartsTab);
        auto* mapVL = new QVBoxLayout(mapPanel);
        mapVL->setContentsMargins(0, 0, 0, 0);
        mapVL->setSpacing(4);
        auto* mapBtnBar = new QHBoxLayout;
        auto* mapFitBtn  = new QPushButton("Fit to Track", mapPanel);
        auto* mapFitSelBtn = new QPushButton("Fit Map To Selection", mapPanel);
        auto* mapZoomIn  = new QPushButton("+", mapPanel);
        auto* mapZoomOut = new QPushButton("-", mapPanel);
        m_mapStyleCombo = new QComboBox(mapPanel);
        m_mapStyleCombo->addItem("Light", static_cast<int>(MapStyle::Light));
        m_mapStyleCombo->addItem("Street Map", static_cast<int>(MapStyle::Street));
        m_mapStyleCombo->addItem("Satellite", static_cast<int>(MapStyle::Satellite));
        m_mapStyleCombo->addItem("Dark", static_cast<int>(MapStyle::Dark));
        m_mapStyleCombo->addItem("Terrain", static_cast<int>(MapStyle::Terrain));
        m_mapStyleCombo->addItem("Topographic", static_cast<int>(MapStyle::Topographic));
        m_mapStyleCombo->addItem("Minimal", static_cast<int>(MapStyle::Minimal));
        m_mapAutoContrastCheck = new QCheckBox("Auto route contrast", mapPanel);
        m_mapAutoContrastCheck->setChecked(true);
        mapZoomIn->setFixedWidth(32);
        mapZoomOut->setFixedWidth(32);
        connect(mapFitBtn, &QPushButton::clicked, m_mapRenderer, &MapRenderer::fitToTrack);
        connect(mapFitSelBtn, &QPushButton::clicked, m_mapRenderer, &MapRenderer::fitToVisibleRange);
        connect(mapZoomIn, &QPushButton::clicked, m_mapRenderer, &MapRenderer::zoomIn);
        connect(mapZoomOut, &QPushButton::clicked, m_mapRenderer, &MapRenderer::zoomOut);
        connect(m_mapStyleCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int)
        {
            if (!m_mapRenderer || !m_mapStyleCombo)
                return;
            const MapStyle style = static_cast<MapStyle>(m_mapStyleCombo->currentData().toInt());
            m_mapRenderer->setMapStyle(style);
        });
        connect(m_mapAutoContrastCheck, &QCheckBox::toggled, this, [this](bool checked)
        {
            if (m_mapRenderer)
                m_mapRenderer->setAutoRouteContrast(checked);
        });
        mapBtnBar->addWidget(mapFitBtn);
        mapBtnBar->addWidget(mapFitSelBtn);
        mapBtnBar->addWidget(mapZoomIn);
        mapBtnBar->addWidget(mapZoomOut);
        mapBtnBar->addSpacing(8);
        mapBtnBar->addWidget(new QLabel("Map Style:", mapPanel));
        mapBtnBar->addWidget(m_mapStyleCombo);
        mapBtnBar->addWidget(m_mapAutoContrastCheck);
        mapBtnBar->addStretch(1);
        mapVL->addLayout(mapBtnBar);
        mapVL->addWidget(m_mapRenderer, 1);

        m_intervalsTable = new QTableWidget(0, 8, chartsTab);
        m_intervalsTable->setHorizontalHeaderLabels(
            { "Type", "Start", "Duration", "Avg W", "NP", "IF", "Max W", "Avg HR" });
        m_intervalsTable->horizontalHeader()->setStretchLastSection(true);
        m_intervalsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_intervalsTable->setAlternatingRowColors(true);
        m_intervalsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_intervalsTable->setSelectionMode(QAbstractItemView::SingleSelection);
        m_intervalsTable->setStyleSheet(
            "QTableWidget::item:selected { background-color: #bfdbfe; color: #0f172a; }");
        connect(m_intervalsTable, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::onIntervalSelectionChanged);
        connect(m_intervalsTable, &QTableWidget::itemDoubleClicked,
            this, &MainWindow::onIntervalRowDoubleClicked);

        m_lapsTable = new QTableWidget(0, 3, chartsTab);
        m_lapsTable->setHorizontalHeaderLabels({ "Lap", "Start", "Duration" });
        m_lapsTable->horizontalHeader()->setStretchLastSection(true);
        m_lapsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

        m_activityNotesView = new QTextEdit(chartsTab);
        m_activityNotesView->setReadOnly(true);
        m_activityNotesView->setPlaceholderText("Activity notes and conditions");

        auto* intervalsPanel = new QWidget(chartsTab);
        auto* intervalsVL = new QVBoxLayout(intervalsPanel);
        intervalsVL->setContentsMargins(0, 0, 0, 0);
        intervalsVL->setSpacing(4);
        auto* intervalsLabel = new QLabel("Intervals", intervalsPanel);
        intervalsLabel->setStyleSheet("font-weight: 600;");
        intervalsVL->addWidget(intervalsLabel);
        auto* intervalsToolbar = new QHBoxLayout;
        auto* saveAutoBtn = new QPushButton("Save Auto Intervals", intervalsPanel);
        auto* addManualBtn = new QPushButton("Add Manual Interval", intervalsPanel);
        m_followIntervalSelectionCheck = new QCheckBox("Follow interval selection", intervalsPanel);
        m_followIntervalSelectionCheck->setChecked(true);
        intervalsToolbar->addWidget(saveAutoBtn);
        intervalsToolbar->addWidget(addManualBtn);
        intervalsToolbar->addWidget(m_followIntervalSelectionCheck);
        intervalsToolbar->addStretch(1);
        intervalsVL->addLayout(intervalsToolbar);
        intervalsVL->addWidget(m_intervalsTable, 1);
        m_intervalSummaryLabel = new QLabel("Interval summary: select an interval", intervalsPanel);
        m_intervalSummaryLabel->setStyleSheet("color: #334155;");
        intervalsVL->addWidget(m_intervalSummaryLabel);

        auto* lapsPanel = new QWidget(chartsTab);
        auto* lapsVL = new QVBoxLayout(lapsPanel);
        lapsVL->setContentsMargins(0, 0, 0, 0);
        lapsVL->setSpacing(4);
        auto* lapsLabel = new QLabel("Laps", lapsPanel);
        lapsLabel->setStyleSheet("font-weight: 600;");
        lapsVL->addWidget(lapsLabel);
        lapsVL->addWidget(m_lapsTable, 1);

        auto* notesPanel = new QWidget(chartsTab);
        auto* notesVL = new QVBoxLayout(notesPanel);
        notesVL->setContentsMargins(0, 0, 0, 0);
        notesVL->setSpacing(4);
        auto* notesLabel = new QLabel("Notes", notesPanel);
        notesLabel->setStyleSheet("font-weight: 600;");
        notesVL->addWidget(notesLabel);
        notesVL->addWidget(m_activityNotesView, 1);

        auto* topSplit = new QSplitter(Qt::Horizontal, chartsTab);
        m_chartMapSplit = topSplit;
        topSplit->addWidget(m_chartScroll);
        topSplit->addWidget(mapPanel);
        topSplit->setStretchFactor(0, 1);
        topSplit->setStretchFactor(1, 1);
        topSplit->setCollapsible(0, false);
        topSplit->setCollapsible(1, false);
        
        // Initialize 50/50 split after layout is complete and keep it balanced
        // when the available width changes.
        QTimer::singleShot(0, topSplit, [topSplit]()
        {
            const QList<int> sizes = topSplit->sizes();
            const int totalWidth = std::accumulate(sizes.begin(), sizes.end(), 0);
            if (totalWidth > 0)
                topSplit->setSizes({ totalWidth / 2, totalWidth / 2 });
        });

        auto* bottomSplit = new QSplitter(Qt::Horizontal, chartsTab);
        bottomSplit->addWidget(intervalsPanel);
        bottomSplit->addWidget(lapsPanel);
        bottomSplit->addWidget(notesPanel);
        bottomSplit->setStretchFactor(0, 2);
        bottomSplit->setStretchFactor(1, 1);
        bottomSplit->setStretchFactor(2, 2);

        auto* activitySplit = new QSplitter(Qt::Vertical, chartsTab);
        activitySplit->addWidget(topSplit);
        activitySplit->addWidget(bottomSplit);
        activitySplit->setStretchFactor(0, 3);
        activitySplit->setStretchFactor(1, 2);

        chartsVL->addWidget(activitySplit, 1);

        m_analysisTabWidget->addTab(chartsTab, "Activity");

        // Checkbox <-> chart visibility; adjustSize keeps container height in sync
        auto makeVisToggle = [this](QCheckBox* cb, RideChartWidget* chart) {
            connect(cb, &QCheckBox::toggled, this, [this, chart](bool v) {
                chart->setVisible(v);
                if (m_chartsContainer) m_chartsContainer->adjustSize();
            });
        };
        makeVisToggle(m_showPower,    m_powerChart);
        makeVisToggle(m_showHR,       m_hrChart);
        makeVisToggle(m_showCadence,  m_cadenceChart);
        makeVisToggle(m_showSpeed,    m_speedChart);
        makeVisToggle(m_showAltitude, m_altitudeChart);

        // X-zoom sync
        const std::initializer_list<RideChartWidget*> all =
            { m_powerChart, m_hrChart, m_cadenceChart, m_speedChart, m_altitudeChart };
        for (auto* src : all)
            for (auto* dst : all)
                if (src != dst)
                    connect(src, &RideChartWidget::xRangeChanged,
                            dst, &RideChartWidget::setXRange);

        // Crosshair sync
        for (auto* src : all)
            for (auto* dst : all)
                if (src != dst)
                    connect(src, &RideChartWidget::crosshairMoved,
                            dst, &RideChartWidget::setCrosshair);

        for (auto* src : all)
        {
            connect(src, &RideChartWidget::crosshairMoved,
                    this, [this](double xMinutes)
            {
                if (!m_mapRenderer)
                    return;
                m_mapRenderer->setCurrentTime(xMinutes < 0.0 ? -1.0 : xMinutes * 60.0);
            });
        }

        connect(m_powerChart, &RideChartWidget::visibleRangeChanged,
                this, [this](double startMinutes, double endMinutes)
        {
            if (!m_mapRenderer)
                return;
            m_mapRenderer->setVisibleTimeRange(startMinutes * 60.0, endMinutes * 60.0);
        });

        auto syncChartsToMapSelection = [this](double startSeconds, double endSeconds)
        {
            const double startMinutes = std::min(startSeconds, endSeconds) / 60.0;
            const double endMinutes = std::max(startSeconds, endSeconds) / 60.0;
            m_powerChart->setXRange(startMinutes, endMinutes);
            m_hrChart->setXRange(startMinutes, endMinutes);
            m_cadenceChart->setXRange(startMinutes, endMinutes);
            m_speedChart->setXRange(startMinutes, endMinutes);
            m_altitudeChart->setXRange(startMinutes, endMinutes);
        };

        connect(m_mapRenderer, &MapRenderer::segmentSelectionChanged,
                this, [syncChartsToMapSelection](double startSeconds, double endSeconds)
        {
            syncChartsToMapSelection(startSeconds, endSeconds);
        });

        connect(m_mapRenderer, &MapRenderer::segmentSelectionFinished,
                this, [syncChartsToMapSelection](double startSeconds, double endSeconds)
        {
            syncChartsToMapSelection(startSeconds, endSeconds);
        });

        connect(m_powerChart, &RideChartWidget::intervalSelectionRequested,
                this, [this](double startSec, double endSec)
        {
            if (!m_dbManager.isOpen() || m_controller->currentActivityId() <= 0)
                return;

            IntervalEditorDialog dlg(startSec, endSec, this);
            if (dlg.exec() != QDialog::Accepted)
                return;

            auto db = m_dbManager.database();
            IntervalRepository repo(db);

            const auto& records = m_controller->rideData().records;
            double sumPower = 0.0;
            int countPower = 0;
            double sumHr = 0.0;
            int countHr = 0;
            double sumCadence = 0.0;
            int countCadence = 0;
            std::vector<double> powerValues;
            for (const RideRecord& r : records)
            {
                if (r.elapsedSeconds < startSec || r.elapsedSeconds > endSec)
                    continue;
                if (r.hasPower)
                {
                    sumPower += r.power;
                    ++countPower;
                    powerValues.push_back(r.power);
                }
                if (r.hasHeartRate)
                {
                    sumHr += r.heartRate;
                    ++countHr;
                }
                if (r.hasCadence)
                {
                    sumCadence += r.cadence;
                    ++countCadence;
                }
            }

            double np = 0.0;
            if (!powerValues.empty())
            {
                double sum4 = 0.0;
                for (double p : powerValues)
                    sum4 += std::pow(p, 4.0);
                np = std::pow(sum4 / static_cast<double>(powerValues.size()), 0.25);
            }

            IntervalRecord record;
            record.activityId = m_controller->currentActivityId();
            record.startSample = static_cast<int>(std::round(startSec));
            record.endSample = static_cast<int>(std::round(endSec));
            record.name = dlg.intervalName().isEmpty() ? QStringLiteral("Manual Interval") : dlg.intervalName();
            record.type = dlg.intervalType().isEmpty() ? QStringLiteral("Manual") : dlg.intervalType();
            record.avgPower = countPower > 0 ? (sumPower / static_cast<double>(countPower)) : 0.0;
            record.np = np;
            record.avgHr = countHr > 0 ? (sumHr / static_cast<double>(countHr)) : 0.0;
            record.avgCadence = countCadence > 0 ? (sumCadence / static_cast<double>(countCadence)) : 0.0;
            record.notes = dlg.intervalNotes();
            repo.insertInterval(record);
            updateIntervals();
            statusBar()->showMessage("Manual interval saved from chart selection.", 2200);
        });

        connect(saveAutoBtn, &QPushButton::clicked, this, [this]
        {
            if (!m_dbManager.isOpen() || m_controller->currentActivityId() <= 0)
                return;

            auto db = m_dbManager.database();
            IntervalRepository repo(db);
            repo.deleteIntervalsForActivity(m_controller->currentActivityId());
            for (const Interval& iv : m_controller->intervals())
            {
                IntervalRecord record;
                record.activityId = m_controller->currentActivityId();
                record.startSample = static_cast<int>(std::round(iv.startSeconds));
                record.endSample = static_cast<int>(std::round(iv.endSeconds));
                record.name = QString::fromStdString(iv.label);
                record.type = QString::fromStdString(iv.label);
                record.avgPower = iv.averagePower;
                record.np = iv.normalizedPower;
                record.avgHr = iv.averageHeartRate;
                record.avgCadence = iv.averageCadence;
                repo.insertInterval(record);
            }
            updateIntervals();
            statusBar()->showMessage("Auto intervals saved.", 2000);
        });

        connect(addManualBtn, &QPushButton::clicked, this, [this]
        {
            if (!m_dbManager.isOpen() || m_controller->currentActivityId() <= 0)
                return;
            const auto& records = m_controller->rideData().records;
            if (records.size() < 2)
                return;

            const double startSec = records[records.size() / 4].elapsedSeconds;
            const double endSec = records[records.size() / 4 + records.size() / 10].elapsedSeconds;

            IntervalEditorDialog dlg(startSec, endSec, this);
            if (dlg.exec() != QDialog::Accepted)
                return;

            auto db = m_dbManager.database();
            IntervalRepository repo(db);
            IntervalRecord record;
            record.activityId = m_controller->currentActivityId();
            record.startSample = static_cast<int>(std::round(startSec));
            record.endSample = static_cast<int>(std::round(endSec));
            record.name = dlg.intervalName();
            record.type = dlg.intervalType().isEmpty() ? QStringLiteral("Manual") : dlg.intervalType();
            record.notes = dlg.intervalNotes();
            repo.insertInterval(record);
            updateIntervals();
            statusBar()->showMessage("Manual interval saved.", 2000);
        });

    }
    // == Tab 1: Power Zones ==
    {
        m_zonesTable = new QTableWidget(0, 5);
        m_zonesTable->setHorizontalHeaderLabels(
            { "Zone", "Name", "Range", "Time in Zone", "%" });
        m_zonesTable->horizontalHeader()->setStretchLastSection(true);
        m_zonesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_zonesTable->setAlternatingRowColors(true);
        m_zonesTable->setSelectionMode(QAbstractItemView::SingleSelection);

        auto* w  = new QWidget;
        auto* vl = new QVBoxLayout(w);
        vl->addWidget(m_zonesTable);
        m_analysisTabWidget->addTab(w, "Zones");
    }

    // == Tab 2: Power Histogram ==
    {
        m_histogram = new PowerHistogramWidget;
        m_analysisTabWidget->addTab(m_histogram, "Histogram");
    }

    // == Tab 3: Power Duration Curve ==
    {
        m_pdcWidget = new PowerCurveWidget;
        m_analysisTabWidget->addTab(m_pdcWidget, "Power Curve");
    }

    // == Tab 4: Calendar ==
    {
        m_calendarWidget = new CalendarWidget(this);
        m_calendarWidget->setDatabaseManager(&m_dbManager);
        m_calendarWidget->setAthleteId(m_currentAthleteId);
        m_analysisTabWidget->addTab(m_calendarWidget, "Calendar");
    }

    // == Tab 5: Fitness/Fatigue/Form ==
    {
        m_fitnessChart = new FitnessChartWidget(this);
        m_analysisTabWidget->addTab(m_fitnessChart, "Fitness");
    }

    analysisLayout->addWidget(m_analysisTabWidget, 1);
    m_tabWidget->addTab(analysisContainer, "Analysis");

    mainLayout->addWidget(m_tabWidget, 1);
    setCentralWidget(central);

    buildToolbar();
    buildStatusBar();

    connect(m_colorMetricCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this](int)
    {
        updateColorLegend();
        updateZoneAvailability();
        updateCharts();
        updateZonesTab();
        if (m_mapRenderer)
            m_mapRenderer->setRideData(m_controller->rideData(), currentColorMetric(), buildColorContext());
    });

    resize(1050, 860);
    setWindowTitle("Fitlyzer C++");
    setWindowIcon(QIcon(":/resources/icons/fitlyzer_logo.png"));
}

void MainWindow::buildToolbar()
{
    auto* tb = addToolBar("Main");
    tb->setMovable(false);
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    m_toolbarImportAct = new QAction("Import", this);
    m_toolbarImportAct->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    m_toolbarImportAct->setEnabled(false);
    connect(m_toolbarImportAct, &QAction::triggered, this, &MainWindow::importActivities);
    tb->addAction(m_toolbarImportAct);

    auto* athleteManageAct = new QAction("Athletes", this);
    athleteManageAct->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    connect(athleteManageAct, &QAction::triggered, this, &MainWindow::manageAthletes);
    tb->addAction(athleteManageAct);

    auto* calendarAct = new QAction("Calendar", this);
    calendarAct->setIcon(style()->standardIcon(QStyle::SP_FileDialogContentsView));
    connect(calendarAct, &QAction::triggered, this, [this]
    {
        if (!m_tabWidget || !m_analysisTabWidget)
            return;
        m_tabWidget->setCurrentIndex(kTabAnalysis);
        m_analysisTabWidget->setCurrentIndex(kAnalysisTabCalendar);
    });
    tb->addAction(calendarAct);

    m_toolbarSearchAct = new QAction("Search", this);
    m_toolbarSearchAct->setIcon(style()->standardIcon(QStyle::SP_FileDialogContentsView));
    connect(m_toolbarSearchAct, &QAction::triggered, this, [this]
    {
        if (m_tabWidget)
            m_tabWidget->setCurrentIndex(kTabActivities);
        if (m_activityBrowser)
            m_activityBrowser->focusSearch();
    });
    tb->addAction(m_toolbarSearchAct);

    tb->addSeparator();
    tb->addWidget(new QLabel("Current Athlete:"));

    m_athleteCombo = new QComboBox(tb);
    m_athleteCombo->setMinimumWidth(220);
    connect(m_athleteCombo, &QComboBox::currentIndexChanged,
            this, &MainWindow::onAthleteSelectionChanged);
    tb->addWidget(m_athleteCombo);
}

void MainWindow::buildStatusBar()
{
    m_dbStatusLabel = new QLabel("Database: none");
    m_athleteStatusLabel = new QLabel("Athlete: none");
    m_activityCountLabel = new QLabel("Activities: 0");

    statusBar()->addPermanentWidget(m_dbStatusLabel);
    statusBar()->addPermanentWidget(m_athleteStatusLabel);
    statusBar()->addPermanentWidget(m_activityCountLabel);
}

// ── Keyboard shortcuts ───────────────────────────────────────────────────────

void MainWindow::setupShortcuts()
{
    // F11: toggle full screen
    auto* fsAct = new QAction(this);
    fsAct->setShortcut(Qt::Key_F11);
    connect(fsAct, &QAction::triggered, this, [this]{
        isFullScreen() ? showNormal() : showFullScreen();
    });
    addAction(fsAct);

    // Ctrl+0: reset zoom on all charts
    auto* resetAct = new QAction(this);
    resetAct->setShortcut(QKeySequence("Ctrl+0"));
    connect(resetAct, &QAction::triggered, this, &MainWindow::resetAllZoom);
    addAction(resetAct);

    if (m_intervalsTable)
    {
        auto* prevIntervalAct = new QAction(m_intervalsTable);
        prevIntervalAct->setShortcut(Qt::Key_Up);
        prevIntervalAct->setShortcutContext(Qt::WidgetShortcut);
        connect(prevIntervalAct, &QAction::triggered, this, [this]
        {
            selectIntervalRowOffset(-1);
        });
        m_intervalsTable->addAction(prevIntervalAct);

        auto* nextIntervalAct = new QAction(m_intervalsTable);
        nextIntervalAct->setShortcut(Qt::Key_Down);
        nextIntervalAct->setShortcutContext(Qt::WidgetShortcut);
        connect(nextIntervalAct, &QAction::triggered, this, [this]
        {
            selectIntervalRowOffset(1);
        });
        m_intervalsTable->addAction(nextIntervalAct);
    }
}

void MainWindow::resetAllZoom()
{
    const std::initializer_list<RideChartWidget*> all =
        { m_powerChart, m_hrChart, m_cadenceChart, m_speedChart, m_altitudeChart };
    for (auto* c : all)
        c->fitToData();
}

void MainWindow::increaseChartHeight()
{
    m_chartHeight += 40;
    applyChartHeight();
}

void MainWindow::decreaseChartHeight()
{
    m_chartHeight = std::max(120, m_chartHeight - 40);
    applyChartHeight();
}

void MainWindow::applyChartHeight()
{
    const std::initializer_list<RideChartWidget*> all =
        { m_powerChart, m_hrChart, m_cadenceChart, m_speedChart, m_altitudeChart };

    int visibleCount = 0;
    for (auto* c : all) {
        c->setFixedHeight(m_chartHeight);
        if (c->isVisible()) ++visibleCount;
        // resizeEvent on the chart will fire and update tick count automatically
    }

    if (!m_chartsContainer) return;

    // Compute the exact pixel height the container needs.
    const int spacing = m_chartsLayout ? m_chartsLayout->spacing() : 4;
    const int totalH  = visibleCount * m_chartHeight
                        + std::max(0, visibleCount - 1) * spacing;
    m_chartsContainer->setFixedHeight(std::max(totalH, 1));

    // Don't set a fixed width - let the splitter handle horizontal sizing.
    // The charts will naturally fill their allocated space in the splitter.
    if (m_chartsContainer)
        m_chartsContainer->setMaximumWidth(QWIDGETSIZE_MAX);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    // When the viewport is resized (including on first show), sync the
    // container width so charts fill the full horizontal space. Calling
    // applyChartHeight() also refreshes the fixed height at that point.
    if (m_chartScroll && watched == m_chartScroll->viewport()
            && event->type() == QEvent::Resize) {
        applyChartHeight();
    }

    if (m_chartMapSplit && watched == m_chartMapSplit && event->type() == QEvent::Resize)
    {
        const QList<int> sizes = m_chartMapSplit->sizes();
        if (sizes.size() == 2 && std::abs(sizes[0] - sizes[1]) > 1)
        {
            const int totalWidth = sizes[0] + sizes[1];
            if (totalWidth > 0)
                m_chartMapSplit->setSizes({ totalWidth / 2, totalWidth / 2 });
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (!event)
        return;

    if (!fitFilesFromMimeData(event->mimeData()).isEmpty())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    if (!event)
        return;

    const QStringList files = fitFilesFromMimeData(event->mimeData());
    if (files.isEmpty())
        return;

    importFiles(files);
    event->acceptProposedAction();
}

void MainWindow::configureFolderWatcher()
{
    if (!m_watcher)
    {
        m_watcher = new QFileSystemWatcher(this);
        connect(m_watcher, &QFileSystemWatcher::directoryChanged,
                this, &MainWindow::onWatchDirectoryChanged);
    }

    if (!m_watchRescanTimer)
    {
        m_watchRescanTimer = new QTimer(this);
        m_watchRescanTimer->setSingleShot(true);
        m_watchRescanTimer->setInterval(700);
        connect(m_watchRescanTimer, &QTimer::timeout, this, [this]
        {
            scanWatchDirectories(false);
        });
    }

    const QStringList current = m_watcher->directories();
    if (!current.isEmpty())
        m_watcher->removePaths(current);

    m_knownWatchedFitFiles.clear();
    if (!m_watchFolderEnabled)
        return;

    const QStringList dirs = monitoredDirectories();
    QStringList validDirs;
    for (const QString& dirPath : dirs)
    {
        const QFileInfo info(dirPath);
        if (info.exists() && info.isDir())
            validDirs << info.absoluteFilePath();
    }

    validDirs.removeDuplicates();
    if (!validDirs.isEmpty())
    {
        m_watcher->addPaths(validDirs);
        scanWatchDirectories(true);
    }
}

QStringList MainWindow::monitoredDirectories() const
{
    QStringList dirs;
    const QString downloads = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    const QString primary = m_watchFolderPath.trimmed().isEmpty() ? downloads : m_watchFolderPath;
    if (!primary.isEmpty())
        dirs << QDir::cleanPath(primary);

    const QString home = QDir::homePath();
    const QStringList candidates = {
        QDir(home).filePath("Downloads/Garmin"),
        QDir(home).filePath("Downloads/Garmin Export"),
        QDir(home).filePath("Garmin/Exports")
    };
    for (const QString& candidate : candidates)
    {
        if (QFileInfo::exists(candidate))
            dirs << QDir::cleanPath(candidate);
    }

    dirs.removeDuplicates();
    return dirs;
}

void MainWindow::scanWatchDirectories(bool initialScan)
{
    if (!m_watchFolderEnabled)
        return;

    const QStringList dirs = monitoredDirectories();
    QSet<QString> currentSet;

    for (const QString& dirPath : dirs)
    {
        QDirIterator it(
            dirPath,
            QStringList() << "*.fit" << "*.FIT",
            QDir::Files,
            QDirIterator::Subdirectories);
        while (it.hasNext())
            currentSet.insert(QDir::cleanPath(it.next()));
    }

    if (initialScan)
    {
        m_knownWatchedFitFiles = currentSet;
        return;
    }

    QStringList newFiles;
    for (const QString& path : currentSet)
    {
        if (!m_knownWatchedFitFiles.contains(path))
            newFiles << path;
    }

    m_knownWatchedFitFiles = currentSet;
    if (newFiles.isEmpty())
        return;

    std::sort(newFiles.begin(), newFiles.end());
    importFilesInternal(newFiles, false, QStringLiteral("Folder watcher"));
}

void MainWindow::onWatchDirectoryChanged(const QString& path)
{
    (void)path;
    if (!m_watchRescanTimer)
        return;
    m_watchRescanTimer->start();
}

// ── Settings ─────────────────────────────────────────────────────────────────

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
        const int presetValue = s.value("chartPreset", ChartPresetCustom).toInt();
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

ColorMetric MainWindow::currentColorMetric() const
{
    if (!m_colorMetricCombo)
        return ColorMetric::None;

    return static_cast<ColorMetric>(m_colorMetricCombo->currentData().toInt());
}

ColorContext MainWindow::buildColorContext() const
{
    ColorContext context;
    context.ftp = static_cast<int>(m_controller->ftp());

    if (m_dbManager.isOpen() && m_currentAthleteId > 0)
    {
        auto db = m_dbManager.database();
        AthleteRepository repo(db);

        int resolvedFtp = 0;
        if (m_controller->currentActivityId() > 0)
        {
            ActivityRepository actRepo(db);
            const Activity activity = actRepo.getActivity(m_controller->currentActivityId());
            const QDate ftpDate = activityDateForFtpContext(activity);
            if (ftpDate.isValid())
                resolvedFtp = repo.getFtpForDate(m_currentAthleteId, ftpDate);
        }

        if (resolvedFtp <= 0)
            resolvedFtp = repo.getAthlete(m_currentAthleteId).ftpWatts;

        if (resolvedFtp > 0)
            context.ftp = resolvedFtp;
    }

    int maxHeartRate = 0;
    double minAltitude = std::numeric_limits<double>::max();
    double maxAltitude = std::numeric_limits<double>::lowest();
    bool hasAltitude = false;

    for (const RideRecord& record : m_controller->rideData().records)
    {
        if (record.hasHeartRate)
            maxHeartRate = std::max(maxHeartRate, static_cast<int>(record.heartRate));
        if (record.hasAltitude)
        {
            minAltitude = std::min(minAltitude, record.altitude);
            maxAltitude = std::max(maxAltitude, record.altitude);
            hasAltitude = true;
        }
    }

    if (maxHeartRate > 0)
        context.heartRateMax = maxHeartRate;
    if (hasAltitude)
    {
        context.altitudeMin = minAltitude;
        context.altitudeMax = maxAltitude;
        context.hasAltitudeRange = true;
    }

    return context;
}

void MainWindow::updateColorLegend()
{
    if (!m_colorLegendLayout || !m_colorLegendWidget)
        return;

    while (QLayoutItem* item = m_colorLegendLayout->takeAt(0))
    {
        if (QWidget* widget = item->widget())
            widget->deleteLater();
        delete item;
    }

    const ColorMetric metric = currentColorMetric();
    if (metric == ColorMetric::None)
    {
        auto* label = new QLabel("Coloring disabled", m_colorLegendWidget);
        label->setStyleSheet("color: #64748b;");
        m_colorLegendLayout->addWidget(label);
        m_colorLegendLayout->addStretch(1);
        return;
    }

    const std::vector<Zone> zones = ZoneCalculator::zonesForMetric(metric, buildColorContext());
    for (const Zone& zone : zones)
    {
        auto* swatch = new QLabel(m_colorLegendWidget);
        swatch->setFixedSize(12, 12);
        swatch->setStyleSheet(QString("background: %1; border: 1px solid rgba(15,23,42,0.15);")
            .arg(zone.color.name()));

        auto* text = new QLabel(zone.name, m_colorLegendWidget);

        auto* chip = new QWidget(m_colorLegendWidget);
        auto* chipLayout = new QHBoxLayout(chip);
        chipLayout->setContentsMargins(0, 0, 0, 0);
        chipLayout->setSpacing(4);
        chipLayout->addWidget(swatch);
        chipLayout->addWidget(text);
        m_colorLegendLayout->addWidget(chip);
    }

    m_colorLegendLayout->addStretch(1);
}

void MainWindow::updateZoneAvailability()
{
    if (!m_analysisTabWidget)
        return;

    const ColorMetric metric = currentColorMetric();
    const bool hasSelectedMetric = ZoneCalculator::hasMetricSamples(m_controller->rideData(), metric);
    m_analysisTabWidget->setTabEnabled(kAnalysisTabZones,
                                       metric != ColorMetric::None && hasSelectedMetric);
}

void MainWindow::importFiles(const QStringList& filePaths)
{
    importFilesInternal(filePaths, true, QStringLiteral("Manual import"));
}

void MainWindow::importFilesInternal(const QStringList& filePaths,
                                     bool showResultDialog,
                                     const QString& sourceLabel)
{
    if (filePaths.isEmpty())
        return;

    if (!ensureImportReady())
    {
        updateImportAvailability();
        return;
    }

    int imported = 0;
    int duplicates = 0;
    int failed = 0;

    for (const QString& filePath : filePaths)
    {
        QString err;
        const int activityId = m_controller->importFile(filePath, err);
        if (activityId > 0)
        {
            ++imported;
            continue;
        }

        if (err.contains("duplicate", Qt::CaseInsensitive) ||
            err.contains("already", Qt::CaseInsensitive))
            ++duplicates;
        else
            ++failed;
    }

    if (m_activityBrowser)
        m_activityBrowser->refresh(m_currentAthleteId);
    updateStatsLabel();
    updateStatusBarInfo();
    updateWelcomeScreenVisibility();

    if (showResultDialog)
    {
        QMessageBox::information(
            this,
            "Import Results",
            QString("%1 files selected\n\nImported: %2\nDuplicates: %3\nFailed: %4")
                .arg(filePaths.size())
                .arg(imported)
                .arg(duplicates)
                .arg(failed));
    }
    else
    {
        statusBar()->showMessage(
            QString("%1: imported %2, duplicates %3, failed %4")
                .arg(sourceLabel.isEmpty() ? "Auto import" : sourceLabel)
                .arg(imported)
                .arg(duplicates)
                .arg(failed),
            4000);
    }
}

QStringList MainWindow::fitFilesFromMimeData(const QMimeData* mimeData) const
{
    QStringList files;
    if (!mimeData || !mimeData->hasUrls())
        return files;

    for (const QUrl& url : mimeData->urls())
    {
        if (!url.isLocalFile())
            continue;
        const QString path = url.toLocalFile();
        if (QFileInfo(path).suffix().compare("fit", Qt::CaseInsensitive) == 0)
            files << path;
    }

    files.removeDuplicates();
    return files;
}

void MainWindow::refreshAthleteSelector()
{
    if (!m_athleteCombo)
        return;

    const QSignalBlocker blocker(m_athleteCombo);
    m_athleteCombo->clear();
    m_athleteCombo->addItem("Select athlete...", -1);

    if (m_dbManager.isOpen())
    {
        auto db = m_dbManager.database();
        AthleteRepository repo(db);
        for (const Athlete& athlete : repo.listAthletes())
            m_athleteCombo->addItem(athlete.fullName(), athlete.id);
    }

    const int idx = m_athleteCombo->findData(m_currentAthleteId);
    if (idx >= 0)
    {
        m_athleteCombo->setCurrentIndex(idx);
    }
    else
    {
        m_athleteCombo->setCurrentIndex(0);
        m_currentAthleteId = -1;
        m_currentAthleteName.clear();
        m_controller->setCurrentAthlete(-1);
    }
}

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
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void MainWindow::importActivities()
{
    const QStringList files = QFileDialog::getOpenFileNames(
            this, "Import Activities",
            m_lastOpenDir,
            "FIT Files (*.fit);;All Files (*)");

    if (files.isEmpty()) return;
    m_lastOpenDir = QFileInfo(files.front()).absolutePath();
    importFiles(files);
}

void MainWindow::previewFitFile()
{
    const QString fileName = QFileDialog::getOpenFileName(
            this, "Preview FIT File",
            m_lastOpenDir,
            "FIT Files (*.fit);;All Files (*)");
    if (fileName.isEmpty()) return;
    m_lastOpenDir = QFileInfo(fileName).absolutePath();

    QString err;
    if (!m_controller->loadFile(fileName, err))
        QMessageBox::critical(this, "Preview Failed", err);
}

void MainWindow::createVideo()
{
    const RideData& rideData = m_controller->rideData();
    if (rideData.records.empty())
    {
        QMessageBox::information(this, "Create Video", "Load an activity before creating a video.");
        return;
    }

    if (m_controller->currentActivityId() <= 0 || !m_dbManager.isOpen())
    {
        QMessageBox::information(this, "Create Video", "Video export is available for saved activities.");
        return;
    }

    auto db = m_dbManager.database();
    ActivityRepository actRepo(db);
    const Activity activity = actRepo.getActivity(m_controller->currentActivityId());

    QString activityName = QFileInfo(activity.fileName).completeBaseName();
    if (activityName.isEmpty())
        activityName = activity.sport;
    if (activityName.isEmpty())
        activityName = QStringLiteral("Ride");

    QString athleteName = m_currentAthleteName.trimmed();
    if (athleteName.isEmpty() && m_currentAthleteId > 0)
    {
        AthleteRepository athleteRepo(db);
        athleteName = athleteRepo.getAthlete(m_currentAthleteId).fullName().trimmed();
    }
    if (athleteName.isEmpty())
        athleteName = QStringLiteral("Athlete");

    const QDate date = !activity.startTime.isEmpty()
        ? QDate::fromString(activity.startTime.left(10), Qt::ISODate)
        : QDate::fromString(activity.importedAt.left(10), Qt::ISODate);
    const QString dateText = DateFormatter::toIsoDate(date.isValid() ? date : QDate::currentDate());

    const QString defaultName = QString("%1-%2-%3.mp4")
        .arg(dateText,
             sanitizeVideoFilePart(activityName),
             sanitizeVideoFilePart(athleteName));
    const QString defaultOutput = QDir(Platform::defaultDatabaseDirectory()).filePath(defaultName);

    const double visibleStartMin = m_powerChart ? m_powerChart->visibleRangeStartMinutes() : 0.0;
    const double visibleEndMin = m_powerChart ? m_powerChart->visibleRangeEndMinutes() : 0.0;

    VideoExportSettings defaults;
    defaults.outputPath = defaultOutput;
    defaults.ffmpegPath = resolveFfmpegExecutablePath();
    defaults.width = 1920;
    defaults.height = 1080;
    defaults.fps = 30;
    defaults.playbackSpeed = 3.0;
    defaults.useSelectedSegment = true;
    defaults.segmentStartSeconds = std::max(0.0, std::min(visibleStartMin, visibleEndMin) * 60.0);
    defaults.segmentEndSeconds = std::max(defaults.segmentStartSeconds,
                                          std::max(visibleStartMin, visibleEndMin) * 60.0);
    defaults.mapStyle = m_mapRenderer ? m_mapRenderer->mapStyle() : MapStyle::Light;
    defaults.autoZoom = true;
    defaults.fixedZoom = 18;
    defaults.followAthlete = true;
    defaults.routeColorMetric = currentColorMetric() == ColorMetric::None
        ? ColorMetric::Power
        : currentColorMetric();
    defaults.athleteName = athleteName;
    defaults.activityName = activityName;
    defaults.colorContext = buildColorContext();
    defaults.rideData = rideData;

    if (defaults.segmentEndSeconds - defaults.segmentStartSeconds < 1.0)
    {
        defaults.segmentStartSeconds = 0.0;
        defaults.segmentEndSeconds = rideData.records.back().elapsedSeconds;
    }

    VideoExportDialog dialog(defaults, this);
    dialog.exec();
}

void MainWindow::onActivityImported(int activityId)
{
    (void)activityId;
    if (!m_firstLaunchCompleted)
    {
        m_firstLaunchCompleted = true;
        QSettings("Fitlyzer", "FitlyzerC").setValue("firstLaunchCompleted", true);
    }

    if (m_activityBrowser)
    {
        m_activityBrowser->refresh(m_currentAthleteId);
        m_tabWidget->setCurrentIndex(kTabActivities);
    }

    updateStatusBarInfo();
    hideWelcomeScreen();
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
    updateImportAvailability();
    updateStatsLabel();
    updateStatusBarInfo();
    updateWelcomeScreenVisibility();
}

void MainWindow::onWorkoutLoaded()
{
    const bool hasPower =
        m_controller->statistics().maximumPower > 0.0;

    updateStatsLabel();
    updateCharts();
    updateColorLegend();
    updateZoneAvailability();

    if (m_analysisTabWidget)
    {
        m_analysisTabWidget->setTabEnabled(kAnalysisTabHistogram, hasPower);
        m_analysisTabWidget->setTabEnabled(kAnalysisTabPDC,       hasPower);
        m_analysisTabWidget->setTabEnabled(kAnalysisTabCalendar,  true);
        m_analysisTabWidget->setTabEnabled(kAnalysisTabFitness,   true);
    }

    if (hasPower)
    {
        updateZonesTab();
        updateHistogram();
        updatePowerCurve();
        updateIntervals();
        updateFitnessChart();
    }

    if (m_mapRenderer)
        m_mapRenderer->setRideData(m_controller->rideData(), currentColorMetric(), buildColorContext());

    // Activity selection should always start from the full-ride chart extent.
    resetAllZoom();
}

// ── Update helpers ────────────────────────────────────────────────────────────

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
        ? QString("FTP %1  Est %2  \u0394%+3")
              .arg(ftpWatts)
              .arg(estimatedFtpRounded)
              .arg(ftpDelta)
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

void MainWindow::updateCharts()
{
    const ColorMetric colorMetric = currentColorMetric();
    const ColorContext colorContext = buildColorContext();

    setUpdatesEnabled(false);
    applyChartSmoothing();
    if (m_powerChart)
    {
        m_powerChart->setIntervals(m_controller->intervals());
    }
    m_powerChart->setRideData(m_controller->rideData(), colorMetric, colorContext);
    m_hrChart->setRideData(m_controller->rideData(), colorMetric, colorContext);
    m_cadenceChart->setRideData(m_controller->rideData(), colorMetric, colorContext);
    m_speedChart->setRideData(m_controller->rideData(), colorMetric, colorContext);
    m_altitudeChart->setRideData(m_controller->rideData(), colorMetric, colorContext);
    setUpdatesEnabled(true);
}

void MainWindow::applyChartPreset(int presetId)
{
    if (!m_colorMetricCombo || !m_powerSmoothingCombo || !m_autoSmoothingCheck)
        return;

    if (presetId == ChartPresetCustom)
        return;

    QSignalBlocker colorBlock(m_colorMetricCombo);
    QSignalBlocker smoothingBlock(m_powerSmoothingCombo);
    QSignalBlocker autoBlock(m_autoSmoothingCheck);

    switch (presetId)
    {
        case ChartPresetPowerAnalysis:
            m_colorMetricCombo->setCurrentIndex(
                m_colorMetricCombo->findData(static_cast<int>(ColorMetric::Power)));
            m_powerSmoothingCombo->setCurrentIndex(m_powerSmoothingCombo->findData(5));
            m_autoSmoothingCheck->setChecked(false);
            m_showPower->setChecked(true);
            m_showHR->setChecked(true);
            m_showCadence->setChecked(true);
            m_showSpeed->setChecked(false);
            m_showAltitude->setChecked(false);
            break;

        case ChartPresetHeartRateFocus:
            m_colorMetricCombo->setCurrentIndex(
                m_colorMetricCombo->findData(static_cast<int>(ColorMetric::HeartRate)));
            m_powerSmoothingCombo->setCurrentIndex(m_powerSmoothingCombo->findData(10));
            m_autoSmoothingCheck->setChecked(true);
            m_showPower->setChecked(true);
            m_showHR->setChecked(true);
            m_showCadence->setChecked(false);
            m_showSpeed->setChecked(false);
            m_showAltitude->setChecked(false);
            break;

        case ChartPresetRaceReview:
            m_colorMetricCombo->setCurrentIndex(
                m_colorMetricCombo->findData(static_cast<int>(ColorMetric::Power)));
            m_powerSmoothingCombo->setCurrentIndex(m_powerSmoothingCombo->findData(30));
            m_autoSmoothingCheck->setChecked(true);
            m_showPower->setChecked(true);
            m_showHR->setChecked(true);
            m_showCadence->setChecked(true);
            m_showSpeed->setChecked(true);
            m_showAltitude->setChecked(true);
            break;

        default:
            return;
    }

    m_powerSmoothingCombo->setEnabled(!m_autoSmoothingCheck->isChecked());
    applyChartSmoothing();
    updateColorLegend();
    updateZoneAvailability();
    updateCharts();
    updateZonesTab();
    if (m_mapRenderer)
        m_mapRenderer->setRideData(m_controller->rideData(), currentColorMetric(), buildColorContext());
}

void MainWindow::applyChartSmoothing()
{
    const std::initializer_list<RideChartWidget*> all =
        { m_powerChart, m_hrChart, m_cadenceChart, m_speedChart, m_altitudeChart };

    const int smoothingSeconds = m_powerSmoothingCombo ? m_powerSmoothingCombo->currentData().toInt() : 0;
    const bool autoSmoothing = m_autoSmoothingCheck ? m_autoSmoothingCheck->isChecked() : false;

    for (auto* chart : all)
    {
        if (!chart)
            continue;
        chart->setPowerSmoothingSeconds(smoothingSeconds);
        chart->setAutoSmoothingEnabled(autoSmoothing);
    }
}

void MainWindow::updateZonesTab()
{
    const ColorMetric metric = currentColorMetric();
    const std::vector<ZoneDistribution> distributions = ZoneCalculator::computeDistribution(
        m_controller->rideData(),
        metric,
        buildColorContext());

    m_zonesTable->setRowCount(static_cast<int>(distributions.size()));

    for (int row = 0; row < static_cast<int>(distributions.size()); ++row)
    {
        const ZoneDistribution& distribution = distributions[static_cast<size_t>(row)];

        auto mkItem = [](const QString& text)
        {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            return item;
        };

        auto* zoneItem = mkItem(QString("Z%1").arg(row + 1));
        zoneItem->setBackground(distribution.zone.color);
        zoneItem->setForeground(QColor("#111827"));
        m_zonesTable->setItem(row, 0, zoneItem);

        auto* nameItem = new QTableWidgetItem(distribution.zone.name);
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_zonesTable->setItem(row, 1, nameItem);

        m_zonesTable->setItem(row, 2, mkItem(rangeLabelForZone(distribution.zone, metric)));

        m_zonesTable->setItem(row, 3, mkItem(fmtDur(distribution.seconds)));
        m_zonesTable->setItem(row, 4, mkItem(QString::number(distribution.percent * 100.0, 'f', 0) + "%"));
    }

    m_zonesTable->resizeColumnsToContents();
}

void MainWindow::updateHistogram()
{
    m_histogram->setRideData(m_controller->rideData());
}

void MainWindow::updatePowerCurve()
{
    if (!m_pdcWidget)
        return;

    std::vector<PowerCurvePoint> last90;
    std::vector<PowerCurvePoint> currentYear;
    std::vector<PowerCurvePoint> allTime;

    if (m_dbManager.isOpen() && m_currentAthleteId > 0)
    {
        auto db = m_dbManager.database();
        ActivityRepository repo(db);
        const QList<Activity> activities = repo.listActivities(m_currentAthleteId);
        const std::vector<double> durations = PowerCurve::standardDurations();

        auto aggregate = [&](auto predicate)
        {
            std::vector<double> best(durations.size(), 0.0);
            for (const Activity& activity : activities)
            {
                QString raw = !activity.startTime.isEmpty() ? activity.startTime.left(10) : activity.importedAt.left(10);
                const QDate day = QDate::fromString(raw, Qt::ISODate);
                if (!predicate(day))
                    continue;

                const RideData ride = RideDataSerializer::loadRideFromDatabase(activity.id, m_dbManager);
                if (ride.records.empty())
                    continue;

                for (size_t i = 0; i < durations.size(); ++i)
                {
                    const double p = PowerCurve::bestMeanPower(ride, durations[i]);
                    if (p > best[i])
                        best[i] = p;
                }
            }

            std::vector<PowerCurvePoint> out;
            for (size_t i = 0; i < durations.size(); ++i)
            {
                if (best[i] > 0.0)
                    out.push_back({durations[i], best[i]});
            }
            return out;
        };

        const QDate today = QDate::currentDate();
        const QDate ninetyAgo = today.addDays(-90);
        const int year = today.year();

        last90 = aggregate([&](const QDate& day)
        {
            return day.isValid() && day >= ninetyAgo;
        });
        currentYear = aggregate([&](const QDate& day)
        {
            return day.isValid() && day.year() == year;
        });
        allTime = aggregate([&](const QDate& day)
        {
            return day.isValid();
        });
    }

    m_pdcWidget->setRideDataWithComparisons(
        m_controller->rideData(),
        last90,
        currentYear,
        allTime,
        m_controller->ftp());
}

double MainWindow::estimatedFtpFromCurrentRide() const
{
    const double best20m = PowerCurve::bestMeanPower(m_controller->rideData(), 20.0 * 60.0);
    if (best20m <= 0.0)
        return 0.0;
    return best20m * 0.95;
}

void MainWindow::applyEstimatedFtpForCurrentAthlete()
{
    if (!m_dbManager.isOpen() || m_currentAthleteId <= 0)
        return;

    const int estimated = static_cast<int>(std::round(estimatedFtpFromCurrentRide()));
    if (estimated <= 0)
    {
        QMessageBox::information(this, "Estimated FTP", "Insufficient power data to estimate FTP.");
        return;
    }

    auto db = m_dbManager.database();
    AthleteRepository repo(db);
    Athlete athlete = repo.getAthlete(m_currentAthleteId);
    if (!athlete.isValid())
        return;

    athlete.ftpWatts = estimated;
    if (!repo.updateAthlete(athlete))
    {
        QMessageBox::critical(this, "FTP Update", "Failed to update athlete FTP.");
        return;
    }

    FtpEntry entry;
    entry.athleteId = m_currentAthleteId;
    entry.ftpWatts = estimated;
    entry.effectiveFrom = DateFormatter::toIsoDate(QDate::currentDate());
    entry.notes = "Estimated from best 20-minute power (95%).";
    repo.addFtpEntry(entry);

    m_controller->setCurrentAthlete(m_currentAthleteId);
    updateStatsLabel();
    statusBar()->showMessage(QString("Estimated FTP applied: %1 W").arg(estimated), 3500);
}

void MainWindow::updateFitnessChart()
{
    if (!m_fitnessChart)
        return;

    if (!m_dbManager.isOpen() || m_currentAthleteId <= 0)
    {
        m_fitnessChart->clearChart();
        return;
    }

    auto db = m_dbManager.database();
    ActivityRepository repo(db);
    const QList<Activity> activities = repo.listActivities(m_currentAthleteId);
    if (activities.isEmpty())
    {
        m_fitnessChart->clearChart();
        return;
    }

    std::map<QDate, double> tssPerDay;
    for (const Activity& activity : activities)
    {
        QString raw = !activity.startTime.isEmpty() ? activity.startTime.left(10) : activity.importedAt.left(10);
        QDate day = QDate::fromString(raw, Qt::ISODate);
        if (!day.isValid())
            continue;

        const double tss = TrainingLoad::trainingStressScore(
            activity.durationSec,
            activity.normalizedPower,
            m_controller->ftp());
        tssPerDay[day] += tss;
    }

    if (tssPerDay.empty())
    {
        m_fitnessChart->clearChart();
        return;
    }

    const QDate first = tssPerDay.begin()->first;
    const QDate last = tssPerDay.rbegin()->first;
    std::vector<DailyLoadPoint> daily;
    daily.reserve(static_cast<size_t>(first.daysTo(last) + 1));
    for (QDate day = first; day <= last; day = day.addDays(1))
    {
        DailyLoadPoint p;
        auto it = tssPerDay.find(day);
        if (it != tssPerDay.end())
            p.tss = it->second;
        daily.push_back(p);
    }

    const std::vector<FitnessMetricsPoint> timeline = TrainingLoad::fitnessTimeline(daily);
    m_fitnessChart->setTimeline(timeline);
}

void MainWindow::onIntervalSelectionChanged()
{
    if (!m_intervalsTable)
        return;

    const int row = m_intervalsTable->currentRow();
    if (row < 0)
    {
        updateIntervalRowStyles();
        clearIntervalSummary();
        return;
    }

    auto* firstItem = m_intervalsTable->item(row, 0);
    if (!firstItem)
    {
        updateIntervalRowStyles();
        clearIntervalSummary();
        return;
    }

    const QVariant startVar = firstItem->data(kIntervalStartRole);
    const QVariant endVar = firstItem->data(kIntervalEndRole);
    if (!startVar.isValid() || !endVar.isValid())
    {
        updateIntervalRowStyles();
        clearIntervalSummary();
        return;
    }

    const double startSeconds = startVar.toDouble();
    const double endSeconds = endVar.toDouble();

    updateIntervalRowStyles();
    updateIntervalSummary(startSeconds, endSeconds);

    if (m_followIntervalSelectionCheck && m_followIntervalSelectionCheck->isChecked())
        navigateToInterval(startSeconds, endSeconds, false);
}

void MainWindow::onIntervalRowDoubleClicked(QTableWidgetItem* item)
{
    if (!item || !m_intervalsTable)
        return;

    const int row = item->row();
    auto* firstItem = m_intervalsTable->item(row, 0);
    if (!firstItem)
        return;

    const QVariant startVar = firstItem->data(kIntervalStartRole);
    const QVariant endVar = firstItem->data(kIntervalEndRole);
    if (!startVar.isValid() || !endVar.isValid())
        return;

    if (m_followIntervalSelectionCheck && m_followIntervalSelectionCheck->isChecked())
        navigateToInterval(startVar.toDouble(), endVar.toDouble(), true);
}

void MainWindow::navigateToInterval(double startSeconds, double endSeconds, bool exactZoom)
{
    if (endSeconds <= startSeconds)
        return;

    const double duration = endSeconds - startSeconds;
    const double padding = exactZoom ? 0.0 : std::max(duration * 0.10, 5.0);
    const double rangeStart = std::max(0.0, startSeconds - padding);
    const double rangeEnd = endSeconds + padding;

    const std::initializer_list<RideChartWidget*> all =
        { m_powerChart, m_hrChart, m_cadenceChart, m_speedChart, m_altitudeChart };
    for (auto* chart : all)
    {
        if (!chart)
            continue;
        chart->setXRange(rangeStart / 60.0, rangeEnd / 60.0);
        chart->setCrosshair(startSeconds / 60.0);
    }

    if (m_mapRenderer)
    {
        m_mapRenderer->setVisibleTimeRange(startSeconds, endSeconds);
        m_mapRenderer->fitToVisibleRange();
        m_mapRenderer->setCurrentTime(startSeconds);
    }

    const QString modeLabel = exactZoom ? "exact" : "padded";
    statusBar()->showMessage(
        QString("Interval inspection: %1 zoom (%2-%3)")
            .arg(modeLabel)
            .arg(fmtDur(rangeStart))
            .arg(fmtDur(rangeEnd)),
        2500);
}

void MainWindow::selectIntervalRowOffset(int delta)
{
    if (!m_intervalsTable)
        return;

    const int rows = m_intervalsTable->rowCount();
    if (rows <= 0)
        return;

    int row = m_intervalsTable->currentRow();
    if (row < 0)
        row = 0;
    else
        row = std::clamp(row + delta, 0, rows - 1);

    m_intervalsTable->setCurrentCell(row, 0);
}

void MainWindow::updateIntervalRowStyles()
{
    if (!m_intervalsTable)
        return;

    const int selectedRow = m_intervalsTable->currentRow();
    for (int row = 0; row < m_intervalsTable->rowCount(); ++row)
    {
        for (int col = 0; col < m_intervalsTable->columnCount(); ++col)
        {
            auto* item = m_intervalsTable->item(row, col);
            if (!item)
                continue;

            QFont font = item->font();
            font.setBold(row == selectedRow);
            item->setFont(font);
        }
    }
}

void MainWindow::clearIntervalSummary()
{
    if (m_intervalSummaryLabel)
        m_intervalSummaryLabel->setText("Interval summary: select an interval");
}

void MainWindow::updateIntervalSummary(double startSeconds, double endSeconds)
{
    if (!m_intervalSummaryLabel || endSeconds <= startSeconds)
    {
        clearIntervalSummary();
        return;
    }

    const auto& records = m_controller->rideData().records;
    double sumPower = 0.0;
    int countPower = 0;
    double sumHr = 0.0;
    int countHr = 0;
    double sumCadence = 0.0;
    int countCadence = 0;
    double sumSpeed = 0.0;
    int countSpeed = 0;
    double elevationGain = 0.0;
    std::vector<double> powerValues;

    bool havePreviousAltitude = false;
    double previousAltitude = 0.0;

    for (const RideRecord& record : records)
    {
        if (record.elapsedSeconds < startSeconds || record.elapsedSeconds > endSeconds)
            continue;

        if (record.hasPower)
        {
            sumPower += record.power;
            ++countPower;
            powerValues.push_back(record.power);
        }
        if (record.hasHeartRate)
        {
            sumHr += record.heartRate;
            ++countHr;
        }
        if (record.hasCadence)
        {
            sumCadence += record.cadence;
            ++countCadence;
        }
        if (record.hasSpeed)
        {
            sumSpeed += record.speed;
            ++countSpeed;
        }
        if (record.hasAltitude)
        {
            if (havePreviousAltitude)
            {
                const double delta = record.altitude - previousAltitude;
                if (delta > 0.0)
                    elevationGain += delta;
            }
            previousAltitude = record.altitude;
            havePreviousAltitude = true;
        }
    }

    double np = 0.0;
    if (!powerValues.empty())
    {
        double sum4 = 0.0;
        for (double p : powerValues)
            sum4 += std::pow(p, 4.0);
        np = std::pow(sum4 / static_cast<double>(powerValues.size()), 0.25);
    }

    const double avgPower = countPower > 0 ? (sumPower / static_cast<double>(countPower)) : 0.0;
    const double avgHr = countHr > 0 ? (sumHr / static_cast<double>(countHr)) : 0.0;
    const double avgCadence = countCadence > 0 ? (sumCadence / static_cast<double>(countCadence)) : 0.0;
    const double avgSpeed = countSpeed > 0 ? (sumSpeed / static_cast<double>(countSpeed)) : 0.0;

    m_intervalSummaryLabel->setText(
        QString("Interval %1 | Avg P %2 | NP %3 | HR %4 | Cad %5 | Speed %6 | Elev +%7")
            .arg(fmtDur(std::max(0.0, endSeconds - startSeconds)))
            .arg(avgPower > 0.0 ? QString("%1 W").arg(avgPower, 0, 'f', 0) : QString("—"))
            .arg(np > 0.0 ? QString("%1 W").arg(np, 0, 'f', 0) : QString("—"))
            .arg(avgHr > 0.0 ? QString("%1 bpm").arg(avgHr, 0, 'f', 0) : QString("—"))
            .arg(avgCadence > 0.0 ? QString("%1 rpm").arg(avgCadence, 0, 'f', 0) : QString("—"))
            .arg(avgSpeed > 0.0 ? QString("%1 km/h").arg(avgSpeed, 0, 'f', 1) : QString("—"))
            .arg(QString("%1 m").arg(elevationGain, 0, 'f', 0)));
}

void MainWindow::updateIntervals()
{
    if (!m_intervalsTable)
        return;

    const int previousRow = m_intervalsTable->currentRow();
    QSignalBlocker selectionBlocker(m_intervalsTable);

    QList<IntervalRecord> records;
    if (m_dbManager.isOpen() && m_controller->currentActivityId() > 0)
    {
        auto db = m_dbManager.database();
        IntervalRepository repo(db);
        records = repo.listIntervalsForActivity(m_controller->currentActivityId());
    }

    if (records.isEmpty())
    {
        const auto& ivs = m_controller->intervals();
        m_intervalsTable->setRowCount(static_cast<int>(ivs.size()));

        for (int row = 0; row < static_cast<int>(ivs.size()); ++row)
        {
            const auto& iv = ivs[static_cast<size_t>(row)];

            auto mkItem = [](const QString& text, Qt::Alignment align = Qt::AlignCenter)
            {
                auto* item = new QTableWidgetItem(text);
                item->setTextAlignment(align);
                return item;
            };

            auto* typeItem = mkItem(QString::fromStdString(iv.label));
            typeItem->setBackground(
                iv.label == "Work"
                    ? QBrush(QColor(60, 110, 200, 100))
                    : QBrush(QColor(60, 170, 60, 100)));
            typeItem->setData(kIntervalStartRole, iv.startSeconds);
            typeItem->setData(kIntervalEndRole, iv.endSeconds);
            m_intervalsTable->setItem(row, 0, typeItem);

            m_intervalsTable->setItem(row, 1, mkItem(fmtDur(iv.startSeconds)));
            m_intervalsTable->setItem(row, 2, mkItem(fmtDur(iv.durationSeconds)));
            m_intervalsTable->setItem(row, 3, mkItem(iv.averagePower > 0
                ? QString("%1 W").arg(iv.averagePower,   0, 'f', 0) : "\xe2\x80\x94"));
            m_intervalsTable->setItem(row, 4, mkItem(iv.normalizedPower > 0
                ? QString("%1 W").arg(iv.normalizedPower, 0, 'f', 0) : "\xe2\x80\x94"));
            const double intervalIf = (m_controller->ftp() > 0.0 && iv.normalizedPower > 0.0)
                ? (iv.normalizedPower / m_controller->ftp()) : 0.0;
            m_intervalsTable->setItem(row, 5, mkItem(intervalIf > 0.0
                ? QString::number(intervalIf, 'f', 2) : "\xe2\x80\x94"));
            m_intervalsTable->setItem(row, 6, mkItem(iv.maximumPower > 0
                ? QString("%1 W").arg(iv.maximumPower,   0, 'f', 0) : "\xe2\x80\x94"));
            m_intervalsTable->setItem(row, 7, mkItem(iv.averageHeartRate > 0
                ? QString("%1 bpm").arg(iv.averageHeartRate, 0, 'f', 0) : "\xe2\x80\x94"));
        }
    }
    else
    {
        m_intervalsTable->setRowCount(records.size());
        for (int row = 0; row < records.size(); ++row)
        {
            const IntervalRecord& r = records[row];
            auto* typeItem = new QTableWidgetItem(r.type);
            typeItem->setTextAlignment(Qt::AlignCenter);
            typeItem->setData(kIntervalStartRole, static_cast<double>(r.startSample));
            typeItem->setData(kIntervalEndRole, static_cast<double>(r.endSample));
            m_intervalsTable->setItem(row, 0, typeItem);
            m_intervalsTable->setItem(row, 1, new QTableWidgetItem(fmtDur(r.startSample)));
            m_intervalsTable->setItem(row, 2, new QTableWidgetItem(fmtDur(std::max(0, r.endSample - r.startSample))));
            m_intervalsTable->setItem(row, 3, new QTableWidgetItem(r.avgPower > 0 ? QString("%1 W").arg(r.avgPower, 0, 'f', 0) : "\xe2\x80\x94"));
            m_intervalsTable->setItem(row, 4, new QTableWidgetItem(r.np > 0 ? QString("%1 W").arg(r.np, 0, 'f', 0) : "\xe2\x80\x94"));
            const double intervalIf = (m_controller->ftp() > 0.0 && r.np > 0.0)
                ? (r.np / m_controller->ftp()) : 0.0;
            m_intervalsTable->setItem(row, 5, new QTableWidgetItem(intervalIf > 0.0 ? QString::number(intervalIf, 'f', 2) : "\xe2\x80\x94"));
            m_intervalsTable->setItem(row, 6, new QTableWidgetItem("\xe2\x80\x94"));
            m_intervalsTable->setItem(row, 7, new QTableWidgetItem(r.avgHr > 0 ? QString("%1 bpm").arg(r.avgHr, 0, 'f', 0) : "\xe2\x80\x94"));
        }
    }

    m_intervalsTable->resizeColumnsToContents();

    if (m_intervalsTable->rowCount() > 0)
    {
        const int targetRow = std::clamp(previousRow >= 0 ? previousRow : 0, 0, m_intervalsTable->rowCount() - 1);
        m_intervalsTable->setCurrentCell(targetRow, 0);
    }

    selectionBlocker.unblock();
    updateIntervalRowStyles();
    if (m_intervalsTable->currentRow() >= 0)
        onIntervalSelectionChanged();
    else
        clearIntervalSummary();

    if (m_lapsTable)
    {
        m_lapsTable->setRowCount(1);
        m_lapsTable->setItem(0, 0, new QTableWidgetItem("1"));
        m_lapsTable->setItem(0, 1, new QTableWidgetItem("0:00"));
        m_lapsTable->setItem(0, 2, new QTableWidgetItem(fmtDur(m_controller->statistics().durationSeconds)));
        m_lapsTable->resizeColumnsToContents();
    }

    if (m_activityNotesView && m_dbManager.isOpen() && m_controller->currentActivityId() > 0)
    {
        auto db = m_dbManager.database();
        ActivityRepository repo(db);
        const Activity activity = repo.getActivity(m_controller->currentActivityId());

        const QDateTime activityDateUtc = QDateTime::fromString(activity.startTime, Qt::ISODate);
        const QDateTime importDateUtc = QDateTime::fromString(activity.importedAt, Qt::ISODate);
        const QString activityDateText = activityDateUtc.isValid()
            ? DateFormatter::formatDateTime(activityDateUtc.toLocalTime())
            : QStringLiteral("-");
        const QString importDateText = importDateUtc.isValid()
            ? DateFormatter::formatDateTime(importDateUtc.toLocalTime())
            : QStringLiteral("-");

        m_activityNotesView->setPlainText(
            QString("Activity Date: %1\nImported: %2\n\nNotes:\n%3\n\nWeather: %4\nTemperature: %5 C\nRPE: %6\nFatigue: %7\nSleep: %8 h\nBike: %9\nEquipment: %10")
                .arg(activityDateText)
                .arg(importDateText)
                .arg(activity.notes)
                .arg(activity.weather)
                .arg(activity.temperature, 0, 'f', 1)
                .arg(activity.rpe)
                .arg(activity.fatigue)
                .arg(activity.sleepHours, 0, 'f', 1)
                .arg(activity.bike)
                .arg(activity.equipment));
    }
}

void MainWindow::editCurrentActivityProperties()
{
    if (!m_dbManager.isOpen() || m_controller->currentActivityId() <= 0)
        return;

    auto db = m_dbManager.database();
    ActivityRepository repo(db);
    Activity activity = repo.getActivity(m_controller->currentActivityId());
    if (!activity.isValid())
        return;

    EditActivityDialog dlg(activity, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    const Activity updated = dlg.updatedActivity();
    if (!repo.updateActivityProperties(activity.id, updated))
    {
        QMessageBox::critical(this, "Error", "Failed to save activity properties.");
        return;
    }

    statusBar()->showMessage("Activity properties updated.", 2500);
}

void MainWindow::showAbout()
{
    AboutDialog dlg(this);
    dlg.exec();
}
