#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

#include "WorkoutController.h"
#include "core/zones/ZoneDefinition.h"
#include "database/DatabaseManager.h"
#include "database/ActivityRepository.h"
#include "database/AthleteRepository.h"
#include "database/ImportRepository.h"

class RideChartWidget;
class FitnessChartWidget;
class PowerHistogramWidget;
class PowerCurveWidget;
class MapRenderer;
class ActivityBrowser;
class AthleteHeaderWidget;
class CalendarWidget;
class QCloseEvent;
class QMimeData;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
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
    void openRecentDatabase();
    void onAthleteSelectionChanged(int index);

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
    bool canImportActivities() const;
    void updateImportAvailability();
    void importFiles(const QStringList& filePaths);
    QStringList fitFilesFromMimeData(const QMimeData* mimeData) const;
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
    void updateIntervals();
    void onIntervalSelectionChanged();
    void onIntervalRowDoubleClicked(QTableWidgetItem* item);
    void navigateToInterval(double startSeconds, double endSeconds, bool exactZoom);
    void selectIntervalRowOffset(int delta);
    void updateIntervalRowStyles();
    void updateIntervalSummary(double startSeconds, double endSeconds);
    void clearIntervalSummary();
    bool ensureDatabase();
    bool ensureAthlete();
    void updateAthleteLabel();
    void updateStatusBarInfo();
    ColorMetric currentColorMetric() const;
    ColorContext buildColorContext() const;
    void updateColorLegend();
    void updateZoneAvailability();
    void applyChartPreset(int presetId);
    void editCurrentActivityProperties();

    // -- Controller ---------------------------------------------------------
    WorkoutController* m_controller = nullptr;

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

    // -- Toolbar/Header -----------------------------------------------------
    QComboBox* m_athleteCombo = nullptr;
    QComboBox* m_colorMetricCombo = nullptr;
    QComboBox* m_chartPresetCombo = nullptr;
    QComboBox* m_powerSmoothingCombo = nullptr;
    QCheckBox* m_autoSmoothingCheck = nullptr;

    // -- Status bar ---------------------------------------------------------
    QLabel* m_dbStatusLabel       = nullptr;
    QLabel* m_athleteStatusLabel  = nullptr;
    QLabel* m_activityCountLabel  = nullptr;

    // -- Stats panel --------------------------------------------------------
    AthleteHeaderWidget* m_athleteHeader = nullptr;
    QLabel* m_summaryMetricsLabel = nullptr;

    // -- Tab widget ---------------------------------------------------------
    QTabWidget* m_tabWidget = nullptr;
    QTabWidget* m_analysisTabWidget = nullptr;

    static constexpr int kTabActivities = 0;
    static constexpr int kTabAnalysis   = 1;

    static constexpr int kAnalysisTabCharts     = 0;
    static constexpr int kAnalysisTabZones      = 1;
    static constexpr int kAnalysisTabHistogram  = 2;
    static constexpr int kAnalysisTabPDC        = 3;
    static constexpr int kAnalysisTabCalendar   = 4;
    static constexpr int kAnalysisTabFitness    = 5;

    // Charts tab
    QScrollArea*     m_chartScroll      = nullptr;
    QWidget*         m_chartsContainer  = nullptr;
    QVBoxLayout*     m_chartsLayout     = nullptr;
    int              m_chartHeight      = 220;
    RideChartWidget* m_powerChart       = nullptr;
    RideChartWidget* m_hrChart          = nullptr;
    RideChartWidget* m_cadenceChart     = nullptr;
    RideChartWidget* m_speedChart       = nullptr;
    RideChartWidget* m_altitudeChart    = nullptr;

    // Chart controls
    QPushButton* m_fitChartsButton            = nullptr;
    QPushButton* m_chartHeightIncreaseButton  = nullptr;
    QPushButton* m_chartHeightDecreaseButton  = nullptr;
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

    // Integrated map panel
    MapRenderer* m_mapRenderer = nullptr;
    QComboBox* m_mapStyleCombo = nullptr;
    QCheckBox* m_mapAutoContrastCheck = nullptr;

    // Calendar tab
    CalendarWidget* m_calendarWidget = nullptr;

    // Activities tab
    ActivityBrowser* m_activityBrowser = nullptr;
};
