// SPDX-License-Identifier: GPL-3


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
#include <QDoubleSpinBox>
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
#include <QStackedLayout>
#include <QStyle>
#include <QTableWidgetItem>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>
#include <QWidget>
#include <QPainter>

#include <QtCharts/QAbstractAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>

#include "ActivityBrowser.h"
#include "AboutDialog.h"
#include "AthleteHeaderWidget.h"
#include "AthleteDialog.h"
#include "AthleteListDialog.h"
#include "CalendarWidget.h"
#include "CreateDatabaseDialog.h"
#include "EditActivityDialog.h"
#include "ImportStatusWidget.h"
#include "IntervalEditorDialog.h"
#include "NavigationSidebar.h"
#include "WelcomeWidget.h"
#include "controllers/ActivityViewController.h"
#include "controllers/ChartController.h"
#include "controllers/MapController.h"
#include "controllers/ImportController.h"
#include "controllers/NavigationController.h"
#include "controllers/MainWindowActions.h"
#include "charts/FitnessChartWidget.h"
#include "charts/PowerCurveWidget.h"
#include "charts/PowerHistogramWidget.h"
#include "charts/RideChartWidget.h"
#include "core/undo/ClimbEditCommand.h"
#include "analysis/ClimbDetector.h"
#include "analysis/ClimbMetrics.h"
#include "analysis/IntervalDetector.h"
#include "core/settings/AppSettings.h"
#include "core/settings/DateFormatter.h"
#include "database/AthleteRepository.h"
#include "database/ClimbRepository.h"
#include "database/IntervalRepository.h"
#include "maps/MapRenderer.h"
#include "model/RideDataSerializer.h"
#include "platform/Platform.h"
#include "import/ImportProgressModel.h"
#include "video/VideoExportDialog.h"
#include "video/VideoExportSettings.h"
#include "utils/FfmpegPath.h"

#include <cmath>
#include <functional>
#include <map>
#include <numeric>

// ── Helpers ─────────────────────────────────────────────────────────────────

/**
 * @brief Formats duration in seconds to human-readable string.
 * @param seconds Duration in seconds.
 * @return Formatted string as "H:MM:SS" or "M:SS".
 */
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

/**
 * @brief Formats activity date for display with relative labels.
 * @param isoDate ISO date string (YYYY-MM-DD).
 * @return Display string ("Today", "Yesterday", or formatted date).
 */
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

/**
 * @brief Extracts activity date for FTP context filtering.
 * @param activity Activity record.
 * @return Activity date from startTime or importedAt.
 */
static QDate activityDateForFtpContext(const Activity& activity)
{
    const QString rawDate = !activity.startTime.isEmpty()
        ? activity.startTime.left(10)
        : activity.importedAt.left(10);
    return QDate::fromString(rawDate, Qt::ISODate);
}

/**
 * @brief Extracts activity date for weight context filtering.
 * @param activity Activity record.
 * @return Activity date from startTime or importedAt.
 */
static QDate activityDateForWeightContext(const Activity& activity)
{
    const QString rawDate = !activity.startTime.isEmpty()
        ? activity.startTime.left(10)
        : activity.importedAt.left(10);
    return QDate::fromString(rawDate, Qt::ISODate);
}

/// @brief Maximum number of recent databases to remember.
static constexpr int kMaxRecentDatabases = 8;

/**
 * @brief Chart view preset identifiers.
 */
enum ChartPresetId
{
    ChartPresetCustom = 0,           ///< User-defined chart layout
    ChartPresetPowerAnalysis,        ///< Power, heart rate, cadence view
    ChartPresetHeartRateFocus,       ///< Heart rate focused view
    ChartPresetRaceReview            ///< Review/race analysis view
};

/// @brief Model role for interval start time in minutes.
static constexpr int kIntervalStartRole = Qt::UserRole + 1;

/// @brief Model role for interval end time in minutes.
static constexpr int kIntervalEndRole = Qt::UserRole + 2;

/// @brief Model role for climb start time in minutes.
static constexpr int kClimbStartRole = Qt::UserRole + 3;

/// @brief Model role for climb end time in minutes.
static constexpr int kClimbEndRole = Qt::UserRole + 4;

/**
 * @brief Sanitizes string for use in video file names.
 * @param text Text to sanitize.
 * @return Text with invalid filename characters replaced by hyphens.
 */
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

/**
 * @brief Wraps a content widget with a "no activity" state overlay.
 * @param content Widget to show when activity is selected.
 * @param outStack Output pointer to the stacked layout for state control.
 * @param message Message to show when no activity is selected.
 * @return Root widget with stacked layout (empty page as index 0, content as index 1).
 */
static QWidget* wrapWithNoActivityState(
    QWidget* content,
    QStackedLayout** outStack,
    const QString& message = QStringLiteral("No Activity Selected"))
{
    auto* root = new QWidget;
    auto* stack = new QStackedLayout(root);
    stack->setContentsMargins(0, 0, 0, 0);

    auto* emptyPage = new QWidget(root);
    auto* emptyLayout = new QVBoxLayout(emptyPage);
    emptyLayout->setContentsMargins(0, 0, 0, 0);
    auto* label = new QLabel(message, emptyPage);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("color: #64748b; font-size: 16px; font-weight: 600;");
    emptyLayout->addWidget(label, 1);

    stack->addWidget(emptyPage);
    stack->addWidget(content);
    stack->setCurrentIndex(0);

    if (outStack)
        *outStack = stack;
    return root;
}

/**
 * @brief Table item that sorts climb names alphabetically.
 */
class ClimbNameTableItem final : public QTableWidgetItem
{
public:
    /**
     * @brief Constructs a climb name table item.
     * @param text Climb name text.
     */
    explicit ClimbNameTableItem(const QString& text)
        : QTableWidgetItem(text)
    {}

    /**
     * @brief Compares items for sorting (alphabetical).
     * @param other Item to compare with.
     * @return True if this should sort before other.
     */
    bool operator<(const QTableWidgetItem& other) const override
    {
        bool leftOk = false;
        bool rightOk = false;
        const int leftNumber = trailingNumber(text(), &leftOk);
        const int rightNumber = trailingNumber(other.text(), &rightOk);

        if (leftOk && rightOk)
            return leftNumber < rightNumber;

        return QString::localeAwareCompare(text(), other.text()) < 0;
    }

private:
    static int trailingNumber(const QString& text, bool* ok)
    {
        if (ok)
            *ok = false;

        int i = text.size() - 1;
        while (i >= 0 && text[i].isSpace())
            --i;

        if (i < 0 || !text[i].isDigit())
            return 0;

        int end = i;
        while (i >= 0 && text[i].isDigit())
            --i;

        const QString number = text.mid(i + 1, end - i);
        bool converted = false;
        const int value = number.toInt(&converted);
        if (ok)
            *ok = converted;
        return value;
    }
};

/**
 * @brief Table item that sorts by numeric key, with text fallback.
 */
class ClimbSortKeyTableItem final : public QTableWidgetItem
{
public:
    /**
     * @brief Constructs a climb sort key table item.
     * @param text Display text.
     * @param sortKey Numeric sort key (NaN for text-based sorting).
     */
    explicit ClimbSortKeyTableItem(const QString& text, double sortKey)
        : QTableWidgetItem(text)
        , m_sortKey(sortKey)
    {}

    /**
     * @brief Compares items for sorting (numeric key first, then alphabetical).
     * @param other Item to compare with.
     * @return True if this should sort before other.
     */
    bool operator<(const QTableWidgetItem& other) const override
    {
        const auto* rhs = dynamic_cast<const ClimbSortKeyTableItem*>(&other);
        if (!rhs)
            return QString::localeAwareCompare(text(), other.text()) < 0;

        const bool leftNaN = std::isnan(m_sortKey);
        const bool rightNaN = std::isnan(rhs->m_sortKey);
        if (leftNaN != rightNaN)
            return !leftNaN;
        if (leftNaN && rightNaN)
            return QString::localeAwareCompare(text(), rhs->text()) < 0;

        if (m_sortKey == rhs->m_sortKey)
            return QString::localeAwareCompare(text(), rhs->text()) < 0;
        return m_sortKey < rhs->m_sortKey;
    }

private:
    double m_sortKey = std::numeric_limits<double>::quiet_NaN();
};

// ── Constructor ─────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setAcceptDrops(true);

    m_controller = new WorkoutController(this);
    m_controller->setDatabaseManager(&m_dbManager);

    buildUI();

    // Instantiate and configure UI controllers
    m_navigationController = new NavigationController(this);
    m_navigationController->setMainTabWidget(m_tabWidget);
    m_navigationController->setAnalysisTabWidget(m_analysisTabWidget);
    m_navigationSidebar = new NavigationSidebar(this);
    m_navigationController->setNavigationSidebar(m_navigationSidebar);

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

        actsMenu->addSeparator();

        auto* detectClimbsAct = new QAction("Detect Climbs...", this);
        connect(detectClimbsAct, &QAction::triggered, this, &MainWindow::triggerDetectClimbs);
        actsMenu->addAction(detectClimbsAct);

        auto* detectIntervalsAct = new QAction("Detect Intervals...", this);
        connect(detectIntervalsAct, &QAction::triggered, this, &MainWindow::triggerDetectIntervals);
        actsMenu->addAction(detectIntervalsAct);

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
                QSettings("Fitlyzer", "FitlyzerC").setValue("lastActivityId", activityId);
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
            if (!m_chartController)
                return;
            syncChartContextToController(false);
            m_chartController->applyChartSmoothing();
        });

        connect(m_autoSmoothingCheck, &QCheckBox::toggled,
                this, [this](bool checked)
        {
            if (!m_powerSmoothingCombo)
                return;
            m_powerSmoothingCombo->setEnabled(!checked);
            if (!m_chartController)
                return;
            syncChartContextToController(false);
            m_chartController->applyChartSmoothing();
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

        m_analysisTabWidget->addTab(
            wrapWithNoActivityState(chartsTab, &m_activityTabStack),
            "Activity");

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

        const std::initializer_list<RideChartWidget*> all =
            { m_powerChart, m_hrChart, m_cadenceChart, m_speedChart, m_altitudeChart };

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
            record.source = QStringLiteral("manual");
            record.locked = true;
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
            record.source = QStringLiteral("manual");
            record.locked = true;
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
        m_analysisTabWidget->addTab(
            wrapWithNoActivityState(w, &m_zonesTabStack),
            "Zones");
    }

    // == Tab 2: Power Histogram ==
    {
        m_histogram = new PowerHistogramWidget;
        m_analysisTabWidget->addTab(
            wrapWithNoActivityState(m_histogram, &m_histogramTabStack),
            "Histogram");
    }

    // == Tab 3: Power Duration Curve ==
    {
        m_pdcWidget = new PowerCurveWidget;
        m_analysisTabWidget->addTab(
            wrapWithNoActivityState(m_pdcWidget, &m_powerCurveTabStack),
            "Power Curve");
    }

    // == Tab 4: Calendar ==
    {
        m_calendarWidget = new CalendarWidget(this);
        m_calendarWidget->setDatabaseManager(&m_dbManager);
        m_calendarWidget->setAthleteId(m_currentAthleteId);
        m_analysisTabWidget->addTab(
            wrapWithNoActivityState(m_calendarWidget, &m_calendarTabStack),
            "Calendar");
    }

    // == Tab 5: Fitness/Fatigue/Form ==
    {
        m_fitnessChart = new FitnessChartWidget(this);
        m_analysisTabWidget->addTab(
            wrapWithNoActivityState(m_fitnessChart, &m_fitnessTabStack),
            "Fitness");
    }

    // == Tab 6: Climbing ==
    {
        auto* climbingTab = new QWidget(this);
        auto* climbingVL = new QVBoxLayout(climbingTab);
        climbingVL->setContentsMargins(0, 0, 0, 0);
        climbingVL->setSpacing(6);

        m_climbAltitudeChart = new RideChartWidget(Metric::Altitude, climbingTab);
        m_climbPowerChart = new RideChartWidget(Metric::Power, climbingTab);
        m_climbHrChart = new RideChartWidget(Metric::HeartRate, climbingTab);
        m_climbCadenceChart = new RideChartWidget(Metric::Cadence, climbingTab);
        m_climbSpeedChart = new RideChartWidget(Metric::Speed, climbingTab);
        m_climbAltitudeChart->setClimbEditingEnabled(true);
        m_climbAltitudeChart->setMinimumHeight(320);
        m_climbAltitudeChart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_climbPowerChart->hide();
        m_climbHrChart->hide();
        m_climbCadenceChart->hide();
        m_climbSpeedChart->hide();

        auto* climbCharts = new QWidget(climbingTab);
        auto* climbChartsVL = new QVBoxLayout(climbCharts);
        climbChartsVL->setContentsMargins(0, 0, 0, 0);
        climbChartsVL->setSpacing(4);
        climbChartsVL->addWidget(m_climbAltitudeChart);

        auto createQuarterChart = [climbingTab](const QString& title, const QColor& color)
        {
            auto* view = new QChartView(new QChart, climbingTab);
            view->setRenderHint(QPainter::Antialiasing);
            view->setMinimumHeight(160);
            view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            view->chart()->setTitle(title);
            view->chart()->setAnimationOptions(QChart::NoAnimation);
            view->chart()->legend()->hide();
            view->chart()->setBackgroundVisible(false);
            view->setStyleSheet("background: #ffffff; border: 1px solid #e2e8f0; border-radius: 6px;");

            auto* set = new QBarSet(title);
            set->setColor(color);
            *set << 0.0 << 0.0 << 0.0 << 0.0;

            auto* series = new QBarSeries(view->chart());
            series->append(set);
            view->chart()->addSeries(series);

            auto* axisX = new QBarCategoryAxis(view->chart());
            axisX->append({ "Q1", "Q2", "Q3", "Q4" });
            view->chart()->addAxis(axisX, Qt::AlignBottom);
            series->attachAxis(axisX);

            auto* axisY = new QValueAxis(view->chart());
            axisY->setRange(0.0, 1.0);
            view->chart()->addAxis(axisY, Qt::AlignLeft);
            series->attachAxis(axisY);

            return view;
        };

        auto* quarterCharts = new QWidget(climbingTab);
        auto* quarterChartsHL = new QHBoxLayout(quarterCharts);
        quarterChartsHL->setContentsMargins(0, 0, 0, 0);
        quarterChartsHL->setSpacing(6);

        m_climbQuarterPowerChart = createQuarterChart(QStringLiteral("Power by Quarter"), QColor(37, 99, 235));
        m_climbQuarterHrChart = createQuarterChart(QStringLiteral("Heart Rate by Quarter"), QColor(220, 38, 38));
        m_climbQuarterCadenceChart = createQuarterChart(QStringLiteral("Cadence by Quarter"), QColor(5, 150, 105));

        quarterChartsHL->addWidget(m_climbQuarterPowerChart, 1);
        quarterChartsHL->addWidget(m_climbQuarterHrChart, 1);
        quarterChartsHL->addWidget(m_climbQuarterCadenceChart, 1);
        climbChartsVL->addWidget(quarterCharts);

        m_climbMinLengthSpin = new QDoubleSpinBox(climbingTab);
        m_climbMinLengthSpin->setRange(0.1, 20.0);
        m_climbMinLengthSpin->setDecimals(2);
        m_climbMinLengthSpin->setSingleStep(0.1);
        m_climbMinLengthSpin->setValue(0.15);

        m_climbMinGainSpin = new QDoubleSpinBox(climbingTab);
        m_climbMinGainSpin->setRange(1.0, 2000.0);
        m_climbMinGainSpin->setDecimals(0);
        m_climbMinGainSpin->setSingleStep(5.0);
        m_climbMinGainSpin->setValue(10.0);

        m_climbMinGradientSpin = new QDoubleSpinBox(climbingTab);
        m_climbMinGradientSpin->setRange(0.5, 20.0);
        m_climbMinGradientSpin->setDecimals(1);
        m_climbMinGradientSpin->setSingleStep(0.5);
        m_climbMinGradientSpin->setValue(4.0);

        m_climbStartGradientSpin = new QDoubleSpinBox(climbingTab);
        m_climbStartGradientSpin->setRange(0.5, 20.0);
        m_climbStartGradientSpin->setDecimals(1);
        m_climbStartGradientSpin->setSingleStep(0.5);
        m_climbStartGradientSpin->setValue(1.5);

        m_climbDipMetersSpin = new QDoubleSpinBox(climbingTab);
        m_climbDipMetersSpin->setRange(0.0, 200.0);
        m_climbDipMetersSpin->setDecimals(1);
        m_climbDipMetersSpin->setSingleStep(1.0);
        m_climbDipMetersSpin->setValue(10.0);

        m_climbDipDistanceSpin = new QDoubleSpinBox(climbingTab);
        m_climbDipDistanceSpin->setRange(10.0, 2000.0);
        m_climbDipDistanceSpin->setDecimals(0);
        m_climbDipDistanceSpin->setSingleStep(10.0);
        m_climbDipDistanceSpin->setValue(200.0);

        m_climbSmoothingSpin = new QDoubleSpinBox(climbingTab);
        m_climbSmoothingSpin->setRange(1.0, 500.0);
        m_climbSmoothingSpin->setDecimals(0);
        m_climbSmoothingSpin->setSingleStep(5.0);
        m_climbSmoothingSpin->setValue(50.0);

        m_climbOverlayEnabledCheck = new QCheckBox("Overlay", climbingTab);
        m_climbOverlayEnabledCheck->setChecked(false);
        m_climbOverlayEnabledCheck->setStyleSheet(
            "QCheckBox { color: #0f172a; font-weight: 600; }"
            "QCheckBox::indicator { width: 16px; height: 16px; border: 2px solid #334155; background: #ffffff; }"
            "QCheckBox::indicator:checked { background: #0f172a; border-color: #0f172a; }");
        m_climbOverlayMetricCombo = new QComboBox(climbingTab);
        m_climbOverlayMetricCombo->addItem("Power", static_cast<int>(ColorMetric::Power));
        m_climbOverlayMetricCombo->addItem("Heart Rate", static_cast<int>(ColorMetric::HeartRate));
        m_climbOverlayMetricCombo->addItem("Cadence", static_cast<int>(ColorMetric::Cadence));
        m_climbOverlayMetricCombo->addItem("Speed", static_cast<int>(ColorMetric::Speed));
        m_climbOverlayMetricCombo->setCurrentIndex(0);
        m_climbOverlayMetricCombo->setEnabled(false);

        auto* climbParamsInline = new QWidget(analysisContainer);
        auto* climbParamsHL = new QHBoxLayout(climbParamsInline);
        climbParamsHL->setContentsMargins(0, 0, 0, 0);
        climbParamsHL->setSpacing(4);

        auto addParam = [climbParamsHL](const QString& label, QDoubleSpinBox* spin)
        {
            auto* tag = new QLabel(label);
            tag->setStyleSheet("color: #334155;");
            spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
            spin->setFixedWidth(62);
            climbParamsHL->addWidget(tag);
            climbParamsHL->addWidget(spin);
        };

        climbParamsHL->addWidget(new QLabel("Climb:"));
        addParam("Len", m_climbMinLengthSpin);
        addParam("Gain", m_climbMinGainSpin);
        addParam("AvgGrad", m_climbMinGradientSpin);
        addParam("StartGrad", m_climbStartGradientSpin);
        addParam("DipM", m_climbDipMetersSpin);
        addParam("DipDist", m_climbDipDistanceSpin);
        addParam("Smooth", m_climbSmoothingSpin);

        colorBar->insertSpacing(2, 12);
        colorBar->insertWidget(3, climbParamsInline);

                        m_climbsTable = new QTableWidget(0, 19, climbingTab);
        m_climbsTable->setHorizontalHeaderLabels(
            { "Name", "Length (km)", "Gain (m)", "Avg Gradient %", "Max Gradient %",
                            "Duration", "Avg Power", "W/kg", "W/kg Rank", "NP", "Avg HR", "Avg Cadence", "Avg Speed", "VAM", "Fade %", "Decouple %", "Difficulty", "Category", "Shape" });
        m_climbsTable->horizontalHeader()->setStretchLastSection(true);
        m_climbsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_climbsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_climbsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_climbsTable->setAlternatingRowColors(true);
                m_climbsTable->setSortingEnabled(true);
                m_climbsTable->sortItems(0, Qt::AscendingOrder);
        m_climbsTable->setContextMenuPolicy(Qt::CustomContextMenu);

        m_climbSummaryLabel = new QLabel("Climb summary: select a climb", climbingTab);
        m_climbSummaryLabel->setStyleSheet("color: #334155;");

        auto* climbOverlayBar = new QWidget(climbingTab);
        auto* climbOverlayHL = new QHBoxLayout(climbOverlayBar);
        climbOverlayHL->setContentsMargins(0, 0, 0, 0);
        climbOverlayHL->setSpacing(6);
        climbOverlayHL->addWidget(new QLabel("Chart Overlay:", climbOverlayBar));
        climbOverlayHL->addWidget(m_climbOverlayEnabledCheck);
        climbOverlayHL->addWidget(m_climbOverlayMetricCombo);
        climbOverlayHL->addStretch(1);

        auto* climbsBottom = new QWidget(climbingTab);
        auto* climbsBottomVL = new QVBoxLayout(climbsBottom);
        climbsBottomVL->setContentsMargins(0, 0, 0, 0);
        climbsBottomVL->setSpacing(4);
        climbsBottomVL->addWidget(new QLabel("Recognized Climbs", climbsBottom));
        climbsBottomVL->addWidget(m_climbsTable, 1);
        climbsBottomVL->addWidget(m_climbSummaryLabel);

        auto* climbingSplit = new QSplitter(Qt::Vertical, climbingTab);
        climbingSplit->addWidget(climbCharts);
        climbingSplit->addWidget(climbsBottom);
        climbingSplit->setStretchFactor(0, 3);
        climbingSplit->setStretchFactor(1, 2);
        climbingSplit->setCollapsible(0, false);
        climbingSplit->setCollapsible(1, false);

        QTimer::singleShot(0, climbingSplit, [climbingSplit]()
        {
            const QList<int> sizes = climbingSplit->sizes();
            const int total = std::accumulate(sizes.begin(), sizes.end(), 0);
            if (total > 0)
                climbingSplit->setSizes({ (total * 60) / 100, (total * 40) / 100 });
        });

        climbingVL->addWidget(climbOverlayBar);
        climbingVL->addWidget(climbingSplit, 1);

        auto wireDetectionChange = [this](QDoubleSpinBox* spin)
        {
            connect(spin, qOverload<double>(&QDoubleSpinBox::valueChanged),
                    this, [this](double)
            {
                detectClimbsAndRefresh();
            });
        };

        wireDetectionChange(m_climbMinLengthSpin);
        wireDetectionChange(m_climbMinGainSpin);
        wireDetectionChange(m_climbMinGradientSpin);
        wireDetectionChange(m_climbStartGradientSpin);
        wireDetectionChange(m_climbDipMetersSpin);
        wireDetectionChange(m_climbDipDistanceSpin);
        wireDetectionChange(m_climbSmoothingSpin);

        connect(m_climbOverlayEnabledCheck, &QCheckBox::toggled, this, [this](bool checked)
        {
            if (!m_climbOverlayMetricCombo || !m_climbAltitudeChart)
                return;
            m_climbOverlayMetricCombo->setEnabled(checked);
            const ColorMetric metric = static_cast<ColorMetric>(m_climbOverlayMetricCombo->currentData().toInt());
            m_climbAltitudeChart->setMetricOverlay(metric, checked);
        });

        connect(m_climbOverlayMetricCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int)
        {
            if (!m_climbOverlayEnabledCheck || !m_climbOverlayEnabledCheck->isChecked() || !m_climbAltitudeChart || !m_climbOverlayMetricCombo)
                return;
            const ColorMetric metric = static_cast<ColorMetric>(m_climbOverlayMetricCombo->currentData().toInt());
            m_climbAltitudeChart->setMetricOverlay(metric, true);
        });

        connect(m_climbsTable, &QTableWidget::itemSelectionChanged,
                this, &MainWindow::onClimbSelectionChanged);
        connect(m_climbsTable, &QTableWidget::itemDoubleClicked,
            this, &MainWindow::onClimbRowDoubleClicked);
        connect(m_climbsTable, &QWidget::customContextMenuRequested,
            this, &MainWindow::onClimbTableContextMenuRequested);
        connect(m_climbAltitudeChart, &RideChartWidget::climbBoundaryEdited,
            this, &MainWindow::onClimbBoundaryEdited);
        connect(m_climbAltitudeChart, &RideChartWidget::newClimbRequested,
            this, &MainWindow::onNewClimbRequested);
        connect(m_climbAltitudeChart, &RideChartWidget::climbClicked,
            this, [this](int climbIndex)
        {
            if (!m_climbsTable || climbIndex < 0 || climbIndex >= static_cast<int>(m_detectedClimbs.size()))
                return;
            const Climb& c = m_detectedClimbs[static_cast<size_t>(climbIndex)];
            for (int row = 0; row < m_climbsTable->rowCount(); ++row)
            {
                auto* item = m_climbsTable->item(row, 0);
                if (!item) continue;
                if (std::abs(item->data(kClimbStartRole).toDouble() - c.startSeconds) < 0.75 &&
                    std::abs(item->data(kClimbEndRole).toDouble()   - c.endSeconds)   < 0.75)
                {
                    m_climbsTable->setCurrentCell(row, 0);
                    m_climbsTable->scrollToItem(item);
                    break;
                }
            }
        });

        const std::initializer_list<RideChartWidget*> climbChartsAll =
            { m_climbAltitudeChart, m_climbPowerChart, m_climbHrChart, m_climbCadenceChart, m_climbSpeedChart };
        for (auto* src : climbChartsAll)
            for (auto* dst : climbChartsAll)
                if (src != dst)
                    connect(src, &RideChartWidget::xRangeChanged, dst, &RideChartWidget::setXRange);

        for (auto* src : climbChartsAll)
            for (auto* dst : climbChartsAll)
                if (src != dst)
                    connect(src, &RideChartWidget::crosshairMoved, dst, &RideChartWidget::setCrosshair);

        updateClimbQuarterCharts(nullptr);

        m_analysisTabWidget->addTab(
            wrapWithNoActivityState(climbingTab, &m_climbingTabStack),
            "Climbing");
    }

    updateChartAnalysisEmptyStates();

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
        applyChartAndMapUpdates(true, false);
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

    tb->addSeparator();
    tb->addWidget(new QLabel(" "));

    m_importStatusWidget = new ImportStatusWidget(tb);
    if (m_importProgressModel)
        m_importStatusWidget->setModel(m_importProgressModel);
    tb->addWidget(m_importStatusWidget);
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

    // Ctrl+Z: undo
    auto* undoAct = new QAction(this);
    undoAct->setShortcut(QKeySequence::Undo);
    connect(undoAct, &QAction::triggered, m_undoManager, &UndoManager::undo);
    addAction(undoAct);

    // Ctrl+Y / Ctrl+Shift+Z: redo
    auto* redoAct = new QAction(this);
    redoAct->setShortcut(QKeySequence::Redo);
    connect(redoAct, &QAction::triggered, m_undoManager, &UndoManager::redo);
    addAction(redoAct);

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

// ── Settings ─────────────────────────────────────────────────────────────────

// settings and database shell methods moved to MainWindowDatabase.cpp

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

// ── Slots ─────────────────────────────────────────────────────────────────────

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
    QSettings("Fitlyzer", "FitlyzerC").setValue("lastActivityId", activityId);
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

// athlete/settings shell methods moved to MainWindowAthlete.cpp

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


void MainWindow::updateClimbingTab()
{
    if (!m_climbsTable)
        return;

    const auto& ride = m_controller->rideData();
    if (ride.records.empty())
    {
        m_detectedClimbs.clear();
        m_climbsTable->setRowCount(0);
        updateClimbQuarterCharts(nullptr);
        if (m_climbSummaryLabel)
            m_climbSummaryLabel->setText("Climb summary: select a climb");
        return;
    }

    // Load stored climbs from controller (which loaded from DB, or detected+stored on first import)
    m_detectedClimbs = m_controller->climbs();
    applyWeightMetricsToClimbs(m_detectedClimbs, resolveActiveActivityWeightKg());
    refreshClimbViews();
}

void MainWindow::refreshClimbViews(double preferredStartSeconds, double preferredEndSeconds)
{
    if (!m_climbsTable)
        return;

    applyDetectedClimbsToCharts();

    const int previousRow = m_climbsTable->currentRow();
    QSignalBlocker blocker(m_climbsTable);
    m_climbsTable->setSortingEnabled(false);
    m_climbsTable->setRowCount(static_cast<int>(m_detectedClimbs.size()));

    auto mkSortKeyItem = [](const QString& text, double sortKey)
    {
        auto* item = new ClimbSortKeyTableItem(text, sortKey);
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    };

    auto mkNumericItem = [mkSortKeyItem](double value, int decimals, const QString& fallback = QStringLiteral("—"))
    {
        if (std::isnan(value))
            return mkSortKeyItem(fallback, std::numeric_limits<double>::quiet_NaN());
        return mkSortKeyItem(QString::number(value, 'f', decimals), value);
    };

    for (int row = 0; row < static_cast<int>(m_detectedClimbs.size()); ++row)
    {
        const Climb& climb = m_detectedClimbs[static_cast<size_t>(row)];
        auto* nameItem = new ClimbNameTableItem(climb.name.isEmpty() ? QString("Climb %1").arg(row + 1) : climb.name);
        nameItem->setData(kClimbStartRole, climb.startSeconds);
        nameItem->setData(kClimbEndRole, climb.endSeconds);
        nameItem->setBackground(QBrush(QColor(34, 197, 94, 45)));
        m_climbsTable->setItem(row, 0, nameItem);
        m_climbsTable->setItem(row, 1, mkNumericItem(climb.lengthKm, 2));
        m_climbsTable->setItem(row, 2, mkNumericItem(climb.elevationGainM, 0));
        m_climbsTable->setItem(row, 3, mkNumericItem(climb.averageGradient, 1));
        m_climbsTable->setItem(row, 4, mkNumericItem(climb.maximumGradient, 1));
        m_climbsTable->setItem(row, 5, mkSortKeyItem(fmtDur(climb.durationSeconds), climb.durationSeconds));
        m_climbsTable->setItem(row, 6, mkNumericItem(climb.averagePower > 0.0 ? climb.averagePower : std::numeric_limits<double>::quiet_NaN(), 0));
        m_climbsTable->setItem(row, 7, mkNumericItem(climb.averageWattsPerKg > 0.0 ? climb.averageWattsPerKg : std::numeric_limits<double>::quiet_NaN(), 2));
        m_climbsTable->setItem(row, 8, mkSortKeyItem(climb.wattsPerKgRank.isEmpty() ? QStringLiteral("—") : climb.wattsPerKgRank, row));
        m_climbsTable->setItem(row, 9, mkNumericItem(climb.normalizedPower > 0.0 ? climb.normalizedPower : std::numeric_limits<double>::quiet_NaN(), 0));
        m_climbsTable->setItem(row, 10, mkNumericItem(climb.averageHeartRate > 0.0 ? climb.averageHeartRate : std::numeric_limits<double>::quiet_NaN(), 0));
        m_climbsTable->setItem(row, 11, mkNumericItem(climb.averageCadence > 0.0 ? climb.averageCadence : std::numeric_limits<double>::quiet_NaN(), 0));
        m_climbsTable->setItem(row, 12, mkNumericItem(climb.averageSpeed > 0.0 ? climb.averageSpeed : std::numeric_limits<double>::quiet_NaN(), 1));
        m_climbsTable->setItem(row, 13, mkNumericItem(climb.vam > 0.0 ? climb.vam : std::numeric_limits<double>::quiet_NaN(), 0));
        m_climbsTable->setItem(row, 14, mkNumericItem(climb.powerFadePct, 1));
        m_climbsTable->setItem(row, 15, mkNumericItem(climb.aerobicDecouplingPct, 1));
        m_climbsTable->setItem(row, 16, mkNumericItem(climb.difficultyScore, 0));
        m_climbsTable->setItem(row, 17, mkSortKeyItem(climb.category.isEmpty() ? QStringLiteral("—") : climb.category, row));
        m_climbsTable->setItem(row, 18, mkSortKeyItem(climb.shapeClass.isEmpty() ? QStringLiteral("—") : climb.shapeClass, row));
    }

    m_climbsTable->resizeColumnsToContents();

    m_climbsTable->setSortingEnabled(true);
    m_climbsTable->sortItems(0, Qt::AscendingOrder);

    restoreClimbSelection(previousRow, preferredStartSeconds, preferredEndSeconds);

    blocker.unblock();
    updateClimbRowStyles();
    m_suppressClimbAutoZoom = true;
    onClimbSelectionChanged();
    m_suppressClimbAutoZoom = false;
}

void MainWindow::applyDetectedClimbsToCharts()
{
    const auto applyClimbOverlays = [this](RideChartWidget* chart)
    {
        if (chart)
            chart->setClimbs(m_detectedClimbs);
    };

    applyClimbOverlays(m_powerChart);
    applyClimbOverlays(m_hrChart);
    applyClimbOverlays(m_cadenceChart);
    applyClimbOverlays(m_speedChart);
    applyClimbOverlays(m_altitudeChart);
    applyClimbOverlays(m_climbPowerChart);
    applyClimbOverlays(m_climbHrChart);
    applyClimbOverlays(m_climbCadenceChart);
    applyClimbOverlays(m_climbSpeedChart);
    applyClimbOverlays(m_climbAltitudeChart);
}

void MainWindow::restoreClimbSelection(int previousRow, double preferredStartSeconds, double preferredEndSeconds)
{
    if (!m_climbsTable)
        return;

    int targetRow = -1;
    if (preferredStartSeconds >= 0.0 && preferredEndSeconds >= 0.0)
    {
        for (int row = 0; row < m_climbsTable->rowCount(); ++row)
        {
            auto* item = m_climbsTable->item(row, 0);
            if (!item)
                continue;
            const double s = item->data(kClimbStartRole).toDouble();
            const double e = item->data(kClimbEndRole).toDouble();
            if (std::abs(s - preferredStartSeconds) < 0.75 &&
                std::abs(e - preferredEndSeconds) < 0.75)
            {
                targetRow = row;
                break;
            }
        }
    }

    if (targetRow < 0 && m_climbsTable->rowCount() > 0)
        targetRow = std::clamp(previousRow >= 0 ? previousRow : 0, 0, m_climbsTable->rowCount() - 1);

    if (targetRow >= 0)
        m_climbsTable->setCurrentCell(targetRow, 0);
}

void MainWindow::updateClimbQuarterCharts(const Climb* climb)
{
    auto updateChart = [climb](QChartView* view,
                               const QString& title,
                               const QColor& color,
                               const std::function<double(const ClimbQuarterMetrics&)>& metric,
                               int decimals,
                               const QString& axisLabel)
    {
        if (!view || !view->chart())
            return;

        QChart* chart = view->chart();
        chart->setTitle(title);
        chart->removeAllSeries();

        const auto oldAxes = chart->axes();
        for (QAbstractAxis* axis : oldAxes)
        {
            chart->removeAxis(axis);
            delete axis;
        }

        auto* set = new QBarSet(title);
        set->setColor(color);

        double maxValue = 1.0;
        for (int q = 0; q < 4; ++q)
        {
            double value = 0.0;
            if (climb)
            {
                value = metric(climb->quarterMetrics[static_cast<size_t>(q)]);
                if (!std::isfinite(value) || value < 0.0)
                    value = 0.0;
            }

            *set << value;
            maxValue = std::max(maxValue, value);
        }

        auto* series = new QBarSeries(chart);
        series->append(set);
        chart->addSeries(series);

        auto* axisX = new QBarCategoryAxis(chart);
        axisX->append({ QStringLiteral("Q1"), QStringLiteral("Q2"), QStringLiteral("Q3"), QStringLiteral("Q4") });
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);

        auto* axisY = new QValueAxis(chart);
        axisY->setRange(0.0, maxValue * 1.15);
        axisY->setLabelFormat(decimals > 0 ? QString("%%.%1f").arg(decimals) : QString("%.0f"));
        axisY->setTitleText(axisLabel);
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
    };

    updateChart(
        m_climbQuarterPowerChart,
        QStringLiteral("Power by Quarter"),
        QColor(37, 99, 235),
        [](const ClimbQuarterMetrics& m) { return m.averagePower; },
        0,
        QStringLiteral("W"));

    updateChart(
        m_climbQuarterHrChart,
        QStringLiteral("Heart Rate by Quarter"),
        QColor(220, 38, 38),
        [](const ClimbQuarterMetrics& m) { return m.averageHeartRate; },
        0,
        QStringLiteral("bpm"));

    updateChart(
        m_climbQuarterCadenceChart,
        QStringLiteral("Cadence by Quarter"),
        QColor(5, 150, 105),
        [](const ClimbQuarterMetrics& m) { return m.averageCadence; },
        0,
        QStringLiteral("rpm"));
}

void MainWindow::detectClimbsAndRefresh()
{
    if (!m_climbsTable)
        return;

    const auto& ride = m_controller->rideData();
    if (ride.records.empty())
    {
        m_detectedClimbs.clear();
        m_climbsTable->setRowCount(0);
        updateClimbQuarterCharts(nullptr);
        if (m_climbSummaryLabel)
            m_climbSummaryLabel->setText("Climb summary: select a climb");
        return;
    }

    ClimbDetector::Config cfg;
    cfg.minLengthKm = m_climbMinLengthSpin ? m_climbMinLengthSpin->value() : 0.15;
    cfg.minElevationGainM = m_climbMinGainSpin ? m_climbMinGainSpin->value() : 10.0;
    cfg.minAverageGradient = m_climbMinGradientSpin ? m_climbMinGradientSpin->value() : 4.0;
    cfg.startGradient = m_climbStartGradientSpin ? m_climbStartGradientSpin->value() : 1.5;
    cfg.maxDipMeters = m_climbDipMetersSpin ? m_climbDipMetersSpin->value() : 10.0;
    cfg.maxDipDistanceMeters = m_climbDipDistanceSpin ? m_climbDipDistanceSpin->value() : 200.0;
    cfg.elevationSmoothingMeters = m_climbSmoothingSpin ? m_climbSmoothingSpin->value() : 50.0;

    m_detectedClimbs = ClimbDetector::detect(ride, cfg);
    applyWeightMetricsToClimbs(m_detectedClimbs, resolveActiveActivityWeightKg());

    refreshClimbViews();
}

double MainWindow::resolveActiveActivityWeightKg() const
{
    if (!m_dbManager.isOpen() || !m_controller || m_controller->currentActivityId() <= 0)
        return 0.0;

    auto db = m_dbManager.database();
    ActivityRepository activityRepo(db);
    const Activity activity = activityRepo.getActivity(m_controller->currentActivityId());

    if (activity.weightKg > 0.0)
        return activity.weightKg;

    if (m_currentAthleteId <= 0)
        return 0.0;

    const QDate activityDate = activityDateForWeightContext(activity);
    AthleteRepository athleteRepo(db);
    const QList<WeightEntry> weightHistory = athleteRepo.getWeightHistory(m_currentAthleteId);

    if (weightHistory.isEmpty())
        return 0.0;

    if (activityDate.isValid())
    {
        for (const WeightEntry& entry : weightHistory)
        {
            const QDate entryDate = QDate::fromString(entry.effectiveFrom, Qt::ISODate);
            if (!entryDate.isValid() || entryDate <= activityDate)
            {
                if (entry.weightKg > 0.0)
                    return entry.weightKg;
            }
        }
    }

    return weightHistory.front().weightKg > 0.0 ? weightHistory.front().weightKg : 0.0;
}

void MainWindow::assignClimbWeightMetrics(Climb& climb, double riderWeightKg) const
{
    climb.averageWattsPerKg = 0.0;
    climb.wattsPerKgRank.clear();

    if (riderWeightKg <= 0.0 || climb.averagePower <= 0.0)
        return;

    climb.averageWattsPerKg = climb.averagePower / riderWeightKg;
    const double wkg = climb.averageWattsPerKg;

    if (wkg >= 4.5)
        climb.wattsPerKgRank = QStringLiteral("Elite");
    else if (wkg >= 3.8)
        climb.wattsPerKgRank = QStringLiteral("Very Strong");
    else if (wkg >= 3.2)
        climb.wattsPerKgRank = QStringLiteral("Strong");
    else if (wkg >= 2.5)
        climb.wattsPerKgRank = QStringLiteral("Moderate");
    else
        climb.wattsPerKgRank = QStringLiteral("Developing");
}

void MainWindow::applyWeightMetricsToClimbs(std::vector<Climb>& climbs, double riderWeightKg) const
{
    for (Climb& climb : climbs)
        assignClimbWeightMetrics(climb, riderWeightKg);
}
