// SPDX-License-Identifier: GPL-3


#include "MainWindow.h"

#include <QCloseEvent>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <QFileInfo>
#include "ActivityBrowser.h"
#include "NavigationSidebar.h"
#include "pages/ActivitiesPage.h"
#include "controllers/ActivityViewController.h"
#include "controllers/ChartController.h"
#include "controllers/MapController.h"
#include "controllers/ImportController.h"
#include "controllers/NavigationController.h"
#include "controllers/MainWindowActions.h"
#include "database/AthleteRepository.h"
#include "import/ImportProgressModel.h"

static constexpr int kMaxRecentDatabases = 8;

// ── Constructor ─────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setAcceptDrops(true);

    m_controller = new WorkoutController(this);
    m_controller->setDatabaseManager(&m_dbManager);

    buildUI();

    // Instantiate and configure UI controllers.
    // m_navigationSidebar and m_pageStack are created inside buildUI().
    // NavigationController manages the page stack and sidebar connection.
    m_navigationController = new NavigationController(this);
    m_navigationController->setPageStack(m_pageStack);
    m_navigationController->setNavigationSidebar(m_navigationSidebar);

    // Wire ActivitiesPage context toolbar signals.
    if (m_activitiesPage)
    {
        connect(m_activitiesPage, &ActivitiesPage::importRequested,
                this, &MainWindow::importActivities);
        connect(m_activitiesPage, &ActivitiesPage::detectClimbsRequested,
                this, &MainWindow::triggerDetectClimbs);
        connect(m_activitiesPage, &ActivitiesPage::detectIntervalsRequested,
                this, &MainWindow::triggerDetectIntervals);
        connect(m_activitiesPage, &ActivitiesPage::editActivityRequested,
                this, &MainWindow::editCurrentActivityProperties);
        // Recalculate metrics: run analysis queue on selected activity.
        connect(m_activitiesPage, &ActivitiesPage::recalculateMetricsRequested,
                this, [this]
        {
            if (m_controller && m_controller->currentActivityId() > 0 && m_analysisQueue)
                m_analysisQueue->enqueue(m_controller->currentActivityId());
        });
    }

    m_activityViewController = new ActivityViewController(m_controller, &m_dbManager, this);
    m_activityViewController->setAthleteHeaderWidget(m_athleteHeader);
    m_activityViewController->setStatusBarLabels(
        m_dbStatusLabel, m_athleteStatusLabel, m_activityCountLabel);
    m_activityViewController->setAthleteContext(m_currentAthleteId, m_currentAthleteName);

    m_chartController = new ChartController(m_controller, &m_dbManager, this);
    m_chartController->setChartWidgets(
        m_powerChart, m_hrChart, m_cadenceChart, m_speedChart, m_altitudeChart,
        m_histogram, m_pdcWidget, m_fitnessChart);
    m_chartController->setAnalysisTabWidgets(
        m_analysisTabWidget,
        m_activityTabStack, m_zonesTabStack, m_histogramTabStack,
        m_powerCurveTabStack, m_calendarTabStack, m_fitnessTabStack);
    m_chartController->setZonesTable(m_zonesTable);
    m_chartController->setClimbCharts(
        m_climbPowerChart, m_climbHrChart, m_climbCadenceChart,
        m_climbSpeedChart, m_climbAltitudeChart);
    m_chartController->setChartControls(
        m_colorMetricCombo, m_powerSmoothingCombo,
        m_autoSmoothingCheck, m_climbOverlayEnabledCheck,
        m_climbOverlayMetricCombo, m_colorLegendLayout);
    m_chartController->setAthleteId(m_currentAthleteId);

    m_mapController = new MapController(m_controller, &m_dbManager, this);
    m_mapController->setMapRenderer(m_mapRenderer);

    m_importController = new ImportController(m_controller, &m_dbManager, this);
    m_importController->setImportQueue(nullptr); // Set after queue is created below
    m_importController->setProgressModel(nullptr); // Set after model is created below
    m_importController->setStatusWidget(m_importStatusWidget);

    m_actionsManager = new MainWindowActions(this, this);

    connect(m_controller, &WorkoutController::workoutLoaded,
            this, &MainWindow::onWorkoutLoaded);
    connect(m_controller, &WorkoutController::activityImported,
            this, &MainWindow::onActivityImported);

    m_undoManager = new UndoManager(50, this);
    m_analysisQueue = new AnalysisQueue(this);
    m_controller->setAnalysisQueue(m_analysisQueue);
    m_importQueue = new ImportQueue(this);
    m_importProgressModel = new ImportProgressModel(this);
    m_importProgressModel->attachQueue(m_importQueue);

    // Wire import queue and model to import controller
    if (m_importController)
    {
        m_importController->setImportQueue(m_importQueue);
        m_importController->setProgressModel(m_importProgressModel);
    }

    m_importRefreshTimer = new QTimer(this);
    m_importRefreshTimer->setSingleShot(true);
    m_importRefreshTimer->setInterval(500);
    connect(m_importRefreshTimer, &QTimer::timeout, this, [this]()
    {
        if (m_activityBrowser)
            m_activityBrowser->refresh(m_currentAthleteId);
        updateStatsLabel();
        updateStatusBarInfo();
        updateWelcomeScreenVisibility();
    });

    // Status bar feedback from the analysis queue
    connect(m_analysisQueue, &AnalysisQueue::pendingCountChanged, this, [this](int count)
    {
        if (count > 0)
            statusBar()->showMessage(QString("Analyzing %1 activit%2 in background...")
                .arg(count).arg(count == 1 ? "y" : "ies"), 0);
        else
            statusBar()->clearMessage();
    });
    connect(m_analysisQueue, &AnalysisQueue::taskCompleted, this, [this](int activityId)
    {
        // If the completed task is the currently loaded activity, refresh climbing/intervals
        if (m_controller && m_controller->currentActivityId() == activityId)
        {
            m_controller->reloadClimbsFromDatabase();
            m_detectedClimbs = m_controller->climbs();
            applyWeightMetricsToClimbs(m_detectedClimbs, resolveActiveActivityWeightKg());
            refreshClimbViews();
            updateIntervals();
        }
    });

    connect(m_importQueue, &ImportQueue::jobFinished, this,
            [this](const QString&, const QString& batchId, const QString&, int activityId, int)
    {
        if (activityId > 0 && m_analysisQueue)
            m_analysisQueue->enqueue(activityId);

        auto it = m_importBatchSummaries.find(batchId);
        if (it != m_importBatchSummaries.end())
            ++it->imported;

        scheduleActivityBrowserRefresh();
        hideWelcomeScreen();
    });

    connect(m_importQueue, &ImportQueue::jobFailed, this,
            [this](const QString&, const QString& batchId, const QString& filePath,
                   const QString& reason, const QString& errorMessage, int)
    {
        auto it = m_importBatchSummaries.find(batchId);
        if (it != m_importBatchSummaries.end())
        {
            if (reason == QStringLiteral("duplicate") || reason == QStringLiteral("time_overlap"))
                ++it->duplicates;
            else
                ++it->failed;

            if (!errorMessage.isEmpty())
                it->errors << QString("%1: %2").arg(QFileInfo(filePath).fileName(), errorMessage);
        }
    });

    connect(m_importQueue, &ImportQueue::queueIdle, this, [this]()
    {
        scheduleActivityBrowserRefresh();
        finalizeImportBatches();
    });

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
            if (m_analysisQueue)
                m_analysisQueue->setDatabasePath(lastDb);
            if (m_importQueue)
                m_importQueue->setDatabasePath(lastDb);
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

            const int lastActivityId = s.value("lastActivityId", -1).toInt();
            if (lastActivityId > 0)
            {
                QString loadErr;
                if (!m_controller->loadActivity(lastActivityId, loadErr))
                    s.remove("lastActivityId");
            }
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

// buildUI moved to MainWindowUiBuild.cpp

// ── Settings ─────────────────────────────────────────────────────────────────

// settings and database shell methods moved to MainWindowDatabase.cpp

// ── Slots ─────────────────────────────────────────────────────────────────────

// athlete/settings shell methods moved to MainWindowAthlete.cpp

// ── Update helpers ────────────────────────────────────────────────────────────

