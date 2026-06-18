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

// buildUI moved to MainWindowUiBuild.cpp

// ── Settings ─────────────────────────────────────────────────────────────────

// settings and database shell methods moved to MainWindowDatabase.cpp

// ── Slots ─────────────────────────────────────────────────────────────────────

// athlete/settings shell methods moved to MainWindowAthlete.cpp

// ── Update helpers ────────────────────────────────────────────────────────────

