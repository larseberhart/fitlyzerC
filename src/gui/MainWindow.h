// SPDX-License-Identifier: GPL-3


#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QSplitter>
#include <QStackedLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QHash>
#include <QStringList>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

#include "WorkoutController.h"
#include "core/zones/ZoneDefinition.h"
#include "analysis/queue/AnalysisQueue.h"
#include "import/ImportQueue.h"
#include "core/undo/UndoManager.h"
#include "database/DatabaseManager.h"
#include "database/ActivityRepository.h"
#include "database/AthleteRepository.h"
#include "database/ClimbRepository.h"
#include "database/ImportRepository.h"

class RideChartWidget;
class FitnessChartWidget;
class PowerHistogramWidget;
class PowerCurveWidget;
class MapRenderer;
class ActivityBrowser;
class AthleteHeaderWidget;
class CalendarWidget;
class WelcomeWidget;
class ImportStatusWidget;
class ImportProgressModel;
class QCloseEvent;
class QFileSystemWatcher;
class QMimeData;
class QPoint;
class QTimer;
class QChartView;

/**
 * @brief Main application window.
 *
 * Provides integrated UI for activity browsing, analysis, charting, mapping,
 * and athlete/profile management.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructs main window.
     * @param parent Parent widget.
     */
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    /// @brief Handles application close event.
    void closeEvent(QCloseEvent* event) override;

    /// @brief Filters events for custom handling.
    bool eventFilter(QObject* watched, QEvent* event) override;

    /// @brief Accepts drag-enter when the payload contains local `.fit` files.
    void dragEnterEvent(QDragEnterEvent* event) override;

    /// @brief Extracts dropped `.fit` files and forwards them to the import queue.
    void dropEvent(QDropEvent* event) override;

private slots:
    void importActivities();
    void previewFitFile();
    void onWorkoutLoaded();
    void onActivityImported(int activityId);
    void openDatabase();
    void createDatabase();
    void manageAthletes();
    void backupDatabase();
    void openSettingsDialog();
    void openRecentDatabase();
    void onAthleteSelectionChanged(int index);
    void createVideo();
    void showAbout();
    void triggerDetectClimbs();
    void triggerDetectIntervals();
    void revertSelectedClimbToAuto();

private:
    void buildUI();
    void buildToolbar();
    void buildStatusBar();
    void setupShortcuts();
    void saveSettings();
    void loadSettings();
    void addRecentDatabase(const QString& path);
    void updateRecentDatabaseMenu();
    void openDatabasePath(const QString& path);

    /// @brief Returns whether importing is currently allowed.
    ///
    /// Requires an open database and a selected athlete.
    bool canImportActivities() const;

    /// @brief Enables/disables import actions based on current readiness.
    void updateImportAvailability();

    /// @brief Queues a batch of file paths for background import.
    void importFiles(const QStringList& filePaths);

    /// @brief Returns deduplicated local `.fit` files from MIME drop/paste data.
    QStringList fitFilesFromMimeData(const QMimeData* mimeData) const;
    void scheduleActivityBrowserRefresh();

    /// @brief Shows a per-batch import result summary when queue work completes.
    void finalizeImportBatches();
    void refreshAthleteSelector();
    void resetAllZoom();
    void increaseChartHeight();
    void decreaseChartHeight();
    void applyChartHeight();
    void updateStatsLabel();
    void updateCharts();
    void applyChartSmoothing();
    void updateZonesTab();
    void updateHistogram();
    void updatePowerCurve();
    void updateFitnessChart();
    void updateAnalysisEmptyStates();
    void updateIntervals();
    void updateClimbingTab();
    void detectClimbsAndRefresh();
    void refreshClimbViews(double preferredStartSeconds = -1.0, double preferredEndSeconds = -1.0);
    void updateClimbQuarterCharts(const Climb* climb);
    double resolveActiveActivityWeightKg() const;
    void assignClimbWeightMetrics(Climb& climb, double riderWeightKg) const;
    void applyWeightMetricsToClimbs(std::vector<Climb>& climbs, double riderWeightKg) const;
    std::vector<int> selectedClimbIndicesFromTable() const;
    void onClimbTableContextMenuRequested(const QPoint& pos);
    void removeSelectedClimbs();
    void editSelectedClimbBoundaries();
    void joinSelectedClimbs();
    void updateClimbRowStyles();
    void onClimbSelectionChanged();
    void onClimbRowDoubleClicked(QTableWidgetItem* item);
    void onClimbBoundaryEdited(double oldStartSeconds, double oldEndSeconds, double newStartSeconds, double newEndSeconds);
    void onNewClimbRequested(double startSeconds, double endSeconds);
    void onIntervalSelectionChanged();
    void onIntervalRowDoubleClicked(QTableWidgetItem* item);
    void navigateToInterval(double startSeconds, double endSeconds, bool exactZoom);
    void selectIntervalRowOffset(int delta);
    void updateIntervalRowStyles();
    void updateIntervalSummary(double startSeconds, double endSeconds);
    void clearIntervalSummary();
    bool ensureDatabase();
    bool ensureAthlete();

    /// @brief Ensures database path, athlete selection, and queue state are import-ready.
    bool ensureImportReady();
    bool createOrOpenDefaultDatabase();
    QString defaultDatabasePathForAutoCreate() const;
    void showWelcomeScreen();
    void hideWelcomeScreen();
    void updateWelcomeScreenVisibility();
    int activityCount() const;
    void updateAthleteLabel();
    void updateStatusBarInfo();
    ColorMetric currentColorMetric() const;
    ColorContext buildColorContext() const;
    void updateColorLegend();
    void updateZoneAvailability();
    void applyChartPreset(int presetId);
    void editCurrentActivityProperties();
    double estimatedFtpFromCurrentRide() const;
    void applyEstimatedFtpForCurrentAthlete();

    /// @brief Internal import entry point shared by menu action and drag-drop.
    /// @param filePaths Candidate files to import (only `.fit` files are queued).
    /// @param showResultDialog Whether to display completion summary dialogs.
    /// @param sourceLabel Source tag stored in import metadata (e.g. "manual", "drop").
    void importFilesInternal(const QStringList& filePaths,
                             bool showResultDialog,
                             const QString& sourceLabel);
    void configureFolderWatcher();
    QStringList monitoredDirectories() const;
    void scanWatchDirectories(bool initialScan);
    void onWatchDirectoryChanged(const QString& path);

    /// @brief Syncs athlete context to all controllers that need it.
    void syncAthleteContextToControllers();

        // -- Undo/Redo ----------------------------------------------------------
        UndoManager*   m_undoManager    = nullptr;

        // -- Background analysis queue -----------------------------------------
        AnalysisQueue* m_analysisQueue  = nullptr;

        // -- Background import queue -------------------------------------------
        ImportQueue* m_importQueue = nullptr;
        ImportProgressModel* m_importProgressModel = nullptr;

    // -- Controller ---------------------------------------------------------
    WorkoutController* m_controller = nullptr;

    // -- UI Controllers (decomposed from MainWindow) ----------------------
    class ActivityViewController* m_activityViewController = nullptr;
    class ChartController* m_chartController = nullptr;
    class MapController* m_mapController = nullptr;
    class ImportController* m_importController = nullptr;
    class NavigationController* m_navigationController = nullptr;
    class MainWindowActions* m_actionsManager = nullptr;

    // -- Database -----------------------------------------------------------
    DatabaseManager m_dbManager;
    int             m_currentAthleteId = -1;
    QString         m_currentAthleteName;

    // -- Menu ---------------------------------------------------------------
    QMenu*   m_athletesMenu    = nullptr;
    QAction* m_athleteLabelAct = nullptr;
    QAction* m_importAct       = nullptr;
    QAction* m_toolbarImportAct = nullptr;
    QAction* m_toolbarSearchAct = nullptr;
    QAction* m_previewAct      = nullptr;
    QMenu*   m_recentDbMenu    = nullptr;
    QString  m_lastOpenDir;

    // -- Welcome / onboarding ----------------------------------------------
    WelcomeWidget* m_welcomeWidget = nullptr;
    bool m_firstLaunchCompleted = false;

    // -- Folder monitoring --------------------------------------------------
    QFileSystemWatcher* m_watcher = nullptr;
    QTimer* m_watchRescanTimer = nullptr;
    QTimer* m_importRefreshTimer = nullptr;
    bool m_watchFolderEnabled = true;
    QString m_watchFolderPath;
    QSet<QString> m_knownWatchedFitFiles;

    // -- Toolbar/Header -----------------------------------------------------
    QComboBox* m_athleteCombo = nullptr;
    QComboBox* m_colorMetricCombo = nullptr;
    QComboBox* m_chartPresetCombo = nullptr;
    QComboBox* m_powerSmoothingCombo = nullptr;
    QCheckBox* m_autoSmoothingCheck = nullptr;
    ImportStatusWidget* m_importStatusWidget = nullptr;

    // -- Status bar ---------------------------------------------------------
    QLabel* m_dbStatusLabel       = nullptr;
    QLabel* m_athleteStatusLabel  = nullptr;
    QLabel* m_activityCountLabel  = nullptr;

    // -- Stats panel --------------------------------------------------------
    AthleteHeaderWidget* m_athleteHeader = nullptr;

    // -- Tab widget ---------------------------------------------------------
    QTabWidget* m_tabWidget = nullptr;
    QTabWidget* m_analysisTabWidget = nullptr;
    QStackedLayout* m_activityTabStack = nullptr;
    QStackedLayout* m_zonesTabStack = nullptr;
    QStackedLayout* m_histogramTabStack = nullptr;
    QStackedLayout* m_powerCurveTabStack = nullptr;
    QStackedLayout* m_calendarTabStack = nullptr;
    QStackedLayout* m_fitnessTabStack = nullptr;
    QStackedLayout* m_climbingTabStack = nullptr;

    static constexpr int kTabActivities = 0;
    static constexpr int kTabAnalysis   = 1;

    static constexpr int kAnalysisTabCharts     = 0;
    static constexpr int kAnalysisTabZones      = 1;
    static constexpr int kAnalysisTabHistogram  = 2;
    static constexpr int kAnalysisTabPDC        = 3;
    static constexpr int kAnalysisTabCalendar   = 4;
    static constexpr int kAnalysisTabFitness    = 5;
    static constexpr int kAnalysisTabClimbing   = 6;

    // Charts tab
    QScrollArea*     m_chartScroll      = nullptr;
    QWidget*         m_chartsContainer  = nullptr;
    QVBoxLayout*     m_chartsLayout     = nullptr;
    QSplitter*       m_chartMapSplit    = nullptr;
    int              m_chartHeight      = 220;
    RideChartWidget* m_powerChart       = nullptr;
    RideChartWidget* m_hrChart          = nullptr;
    RideChartWidget* m_cadenceChart     = nullptr;
    RideChartWidget* m_speedChart       = nullptr;
    RideChartWidget* m_altitudeChart    = nullptr;

    // Climbing tab charts
    RideChartWidget* m_climbAltitudeChart = nullptr;
    RideChartWidget* m_climbPowerChart = nullptr;
    RideChartWidget* m_climbHrChart = nullptr;
    RideChartWidget* m_climbCadenceChart = nullptr;
    RideChartWidget* m_climbSpeedChart = nullptr;
    QChartView* m_climbQuarterPowerChart = nullptr;
    QChartView* m_climbQuarterHrChart = nullptr;
    QChartView* m_climbQuarterCadenceChart = nullptr;

    // Chart controls
    QPushButton* m_fitChartsButton            = nullptr;
    QPushButton* m_chartHeightIncreaseButton  = nullptr;
    QPushButton* m_chartHeightDecreaseButton  = nullptr;
    QPushButton* m_useEstimatedFtpButton      = nullptr;
    QWidget*     m_colorLegendWidget          = nullptr;
    QHBoxLayout* m_colorLegendLayout          = nullptr;

    // Metric visibility checkboxes
    QCheckBox* m_showPower    = nullptr;
    QCheckBox* m_showHR       = nullptr;
    QCheckBox* m_showCadence  = nullptr;
    QCheckBox* m_showSpeed    = nullptr;
    QCheckBox* m_showAltitude = nullptr;

    // Zones tab
    QTableWidget* m_zonesTable = nullptr;

    // Histogram tab
    PowerHistogramWidget* m_histogram = nullptr;

    // Power curve tab
    PowerCurveWidget* m_pdcWidget = nullptr;

    // Fitness tab
    FitnessChartWidget* m_fitnessChart = nullptr;

    // Integrated activity panels
    QTableWidget* m_intervalsTable = nullptr;
    QTableWidget* m_lapsTable = nullptr;
    QTextEdit* m_activityNotesView = nullptr;
    QCheckBox* m_followIntervalSelectionCheck = nullptr;
    QLabel* m_intervalSummaryLabel = nullptr;

    // Climbing tab
    QTableWidget* m_climbsTable = nullptr;
    QLabel* m_climbSummaryLabel = nullptr;
    QDoubleSpinBox* m_climbMinLengthSpin = nullptr;
    QDoubleSpinBox* m_climbMinGainSpin = nullptr;
    QDoubleSpinBox* m_climbMinGradientSpin = nullptr;
    QDoubleSpinBox* m_climbStartGradientSpin = nullptr;
    QDoubleSpinBox* m_climbDipMetersSpin = nullptr;
    QDoubleSpinBox* m_climbDipDistanceSpin = nullptr;
    QDoubleSpinBox* m_climbSmoothingSpin = nullptr;
    QCheckBox* m_climbOverlayEnabledCheck = nullptr;
    QComboBox* m_climbOverlayMetricCombo = nullptr;
    std::vector<Climb> m_detectedClimbs;
    bool m_suppressClimbAutoZoom = false;

    // Integrated map panel
    MapRenderer* m_mapRenderer = nullptr;
    QComboBox* m_mapStyleCombo = nullptr;
    QCheckBox* m_mapAutoContrastCheck = nullptr;

    // Calendar tab
    CalendarWidget* m_calendarWidget = nullptr;

    // Activities tab
    ActivityBrowser* m_activityBrowser = nullptr;

    struct ImportBatchSummary
    {
        int queued = 0;
        int imported = 0;
        int duplicates = 0;
        int failed = 0;
        bool showResultDialog = false;
        QString sourceLabel;
        QStringList errors;
    };

    QHash<QString, ImportBatchSummary> m_importBatchSummaries;
};