// SPDX-License-Identifier: GPL-3

#include "MainWindow.h"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QSizePolicy>
#include <QSplitter>
#include <QStackedLayout>
#include <QStackedWidget>
#include <QStyle>
#include <QTableWidget>
#include <QTextEdit>
#include <QStatusBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>

#include "AboutDialog.h"
#include "ActivityBrowser.h"
#include "AthleteHeaderWidget.h"
#include "CalendarWidget.h"
#include "IntervalEditorDialog.h"
#include "NavigationSidebar.h"
#include "WelcomeWidget.h"
#include "pages/ActivitiesPage.h"
#include "pages/CalendarPage.h"
#include "pages/ChartsPage.h"
#include "pages/ClimbsPage.h"
#include "pages/FitnessPage.h"
#include "pages/IntervalsPage.h"
#include "pages/PowerPage.h"
#include "pages/VideoPage.h"
#include "analysis/IntervalDetector.h"
#include "charts/FitnessChartWidget.h"
#include "charts/PowerCurveWidget.h"
#include "charts/PowerHistogramWidget.h"
#include "charts/RideChartWidget.h"
#include "database/IntervalRepository.h"
#include "maps/MapRenderer.h"
#include "controllers/ChartController.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace
{
enum ChartPresetId
{
    ChartPresetCustom = 0,
    ChartPresetPowerAnalysis,
    ChartPresetHeartRateFocus,
    ChartPresetRaceReview
};

static constexpr int kClimbStartRole = Qt::UserRole + 3;
static constexpr int kClimbEndRole = Qt::UserRole + 4;

QWidget* wrapWithNoActivityState(
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
} // namespace

void MainWindow::buildUI()
{
    {
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

        auto* actsMenu = menuBar()->addMenu("&Activities");

        m_importAct = new QAction("Import Activities...", this);
        m_importAct->setShortcut(QKeySequence::Open);
        m_importAct->setEnabled(false);
        connect(m_importAct, &QAction::triggered, this, &MainWindow::importActivities);
        actsMenu->addAction(m_importAct);

        m_athletesMenu = menuBar()->addMenu("&Athletes");

        auto* manageAct = new QAction("Manage Athletes...", this);
        connect(manageAct, &QAction::triggered, this, &MainWindow::manageAthletes);
        m_athletesMenu->addAction(manageAct);

        auto* toolsMenu = menuBar()->addMenu("&Tools");
        m_previewAct = new QAction("Preview FIT File...", this);
        connect(m_previewAct, &QAction::triggered, this, &MainWindow::previewFitFile);
        toolsMenu->addAction(m_previewAct);
        toolsMenu->addSeparator();
        auto* settingsAct = new QAction("Settings...", this);
        settingsAct->setShortcut(QKeySequence::Preferences);
        connect(settingsAct, &QAction::triggered, this, &MainWindow::openSettingsDialog);
        toolsMenu->addAction(settingsAct);

        auto* viewMenu = menuBar()->addMenu("&View");
        viewMenu->addAction("Reset Zoom", QKeySequence("Ctrl+0"), this, &MainWindow::resetAllZoom);
        viewMenu->addAction("Increase Chart Height", this, &MainWindow::increaseChartHeight);
        viewMenu->addAction("Decrease Chart Height", this, &MainWindow::decreaseChartHeight);
        auto* helpMenu = menuBar()->addMenu("&Help");
        helpMenu->addAction("&About", this, &MainWindow::showAbout);
    }

    auto* central = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(6);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    m_athleteHeader = new AthleteHeaderWidget(this);
    mainLayout->addWidget(m_athleteHeader);

    m_welcomeWidget = new WelcomeWidget(this);
    mainLayout->addWidget(m_welcomeWidget, 1);
    m_welcomeWidget->setVisible(false);
    connect(m_welcomeWidget, &WelcomeWidget::importRequested, this, &MainWindow::importActivities);
    connect(m_welcomeWidget, &WelcomeWidget::openDatabaseRequested, this, &MainWindow::openDatabase);
    connect(m_welcomeWidget, &WelcomeWidget::createDatabaseRequested, this, &MainWindow::createDatabase);
    connect(m_welcomeWidget, &WelcomeWidget::manageAthletesRequested, this, &MainWindow::manageAthletes);

    // Color-by metric control bar — placed at the top of ChartsPage.
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

    {
        m_activityBrowser = new ActivityBrowser(&m_dbManager, this);

        connect(m_activityBrowser, &ActivityBrowser::activitySelected,
                this, [this](int activityId)
        {
            QString err;
            if (!m_controller->loadActivity(activityId, err))
                QMessageBox::critical(this, "Load Error", err);
            else
            {
                QSettings("Fitlyzer", "FitlyzerC").setValue("lastActivityId", activityId);
                // Navigate to Charts page — the sidebar and NavigationController
                // keep the page stack in sync.
                if (m_navigationSidebar)
                    m_navigationSidebar->setCurrentPage(NavigationSidebar::Page::Charts);
            }
        });

        connect(m_activityBrowser, &ActivityBrowser::activityDeleted,
                this, [this](int activityId)
        {
            (void)activityId;
            updateStatsLabel();
            updateStatusBarInfo();
            updateWelcomeScreenVisibility();
        });
    }

    {
        m_showPower = new QCheckBox("Power");
        m_showPower->setChecked(true);
        m_showHR = new QCheckBox("Heart Rate");
        m_showHR->setChecked(true);
        m_showCadence = new QCheckBox("Cadence");
        m_showCadence->setChecked(true);
        m_showSpeed = new QCheckBox("Speed");
        m_showSpeed->setChecked(true);
        m_showAltitude = new QCheckBox("Altitude");
        m_showAltitude->setChecked(true);

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

        m_fitChartsButton = new QPushButton("Reset Zoom");
        m_chartHeightIncreaseButton = new QPushButton("Increase Height");
        m_chartHeightDecreaseButton = new QPushButton("Decrease Height");
        m_useEstimatedFtpButton = new QPushButton("Use Estimated FTP");
        auto* editActivityButton = new QPushButton("Activity Properties");
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

        m_powerChart = new RideChartWidget(Metric::Power);
        m_hrChart = new RideChartWidget(Metric::HeartRate);
        m_cadenceChart = new RideChartWidget(Metric::Cadence);
        m_speedChart = new RideChartWidget(Metric::Speed);
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

        m_chartsContainer = new QWidget;
        m_chartsLayout = new QVBoxLayout(m_chartsContainer);
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

        auto* chartsTab = new QWidget;
        auto* chartsVL = new QVBoxLayout(chartsTab);
        chartsVL->setContentsMargins(0, 0, 0, 0);
        chartsVL->setSpacing(4);
        chartsVL->addLayout(ctrlBar);

        // m_colorLegendWidget is placed in the Charts page header bar (colorBarWidget),
        // not inline in chartsTab, so it must not be reparented here.
        m_colorLegendWidget = new QWidget(this);
        m_colorLegendLayout = new QHBoxLayout(m_colorLegendWidget);
        m_colorLegendLayout->setContentsMargins(0, 0, 0, 0);
        m_colorLegendLayout->setSpacing(8);
        m_mapRenderer = new MapRenderer;

        auto* mapPanel = new QWidget(chartsTab);
        auto* mapVL = new QVBoxLayout(mapPanel);
        mapVL->setContentsMargins(0, 0, 0, 0);
        mapVL->setSpacing(4);
        auto* mapBtnBar = new QHBoxLayout;
        auto* mapFitBtn = new QPushButton("Fit to Track", mapPanel);
        auto* mapFitSelBtn = new QPushButton("Fit Map To Selection", mapPanel);
        auto* mapZoomIn = new QPushButton("+", mapPanel);
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

        QTimer::singleShot(0, topSplit, [topSplit]()
        {
            const QList<int> sizes = topSplit->sizes();
            const int totalWidth = std::accumulate(sizes.begin(), sizes.end(), 0);
            if (totalWidth > 0)
                topSplit->setSizes({ totalWidth / 2, totalWidth / 2 });
        });

        auto* bottomSplit = new QSplitter(Qt::Horizontal, chartsTab);
        bottomSplit->addWidget(lapsPanel);
        bottomSplit->addWidget(notesPanel);
        bottomSplit->setStretchFactor(0, 1);
        bottomSplit->setStretchFactor(1, 2);

        // Intervals now live on the dedicated Intervals page.
        m_intervalsPageContent = intervalsPanel;

        auto* activitySplit = new QSplitter(Qt::Vertical, chartsTab);
        activitySplit->addWidget(topSplit);
        activitySplit->addWidget(bottomSplit);
        activitySplit->setStretchFactor(0, 3);
        activitySplit->setStretchFactor(1, 2);

        chartsVL->addWidget(activitySplit, 1);

        // Wrap the charts/map/intervals content in an empty-state guard.
        // The resulting widget becomes the content area of ChartsPage.
        // Intervals panel remains here for chart-context navigation (interval
        // selection zooms the chart).  IntervalsPage shows a standalone view.
        m_chartsPageContent = wrapWithNoActivityState(chartsTab, &m_activityTabStack);

        auto makeVisToggle = [this](QCheckBox* cb, RideChartWidget* chart)
        {
            connect(cb, &QCheckBox::toggled, this, [this, chart](bool v)
            {
                chart->setVisible(v);
                if (m_chartsContainer)
                    m_chartsContainer->adjustSize();
            });
        };
        makeVisToggle(m_showPower, m_powerChart);
        makeVisToggle(m_showHR, m_hrChart);
        makeVisToggle(m_showCadence, m_cadenceChart);
        makeVisToggle(m_showSpeed, m_speedChart);
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

    {
        m_zonesTable = new QTableWidget(0, 5);
        m_zonesTable->setHorizontalHeaderLabels(
            { "Zone", "Name", "Range", "Time in Zone", "%" });
        m_zonesTable->horizontalHeader()->setStretchLastSection(true);
        m_zonesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_zonesTable->setAlternatingRowColors(true);
        m_zonesTable->setSelectionMode(QAbstractItemView::SingleSelection);

        auto* w = new QWidget;
        auto* vl = new QVBoxLayout(w);
        vl->addWidget(m_zonesTable);
        // Store wrapped zones widget for the Power page.
        m_zonesPageContent = wrapWithNoActivityState(w, &m_zonesTabStack);
    }

    {
        m_histogram = new PowerHistogramWidget;
        // Store wrapped histogram widget for the Power page.
        m_histogramPageContent = wrapWithNoActivityState(m_histogram, &m_histogramTabStack);
    }

    {
        m_pdcWidget = new PowerCurveWidget;
        // Store wrapped power-curve widget for the Power page.
        m_pdcPageContent = wrapWithNoActivityState(m_pdcWidget, &m_powerCurveTabStack);
    }

    {
        m_calendarWidget = new CalendarWidget(this);
        m_calendarWidget->setDatabaseManager(&m_dbManager);
        m_calendarWidget->setAthleteId(m_currentAthleteId);
        // Store wrapped calendar widget for CalendarPage.
        m_calendarPageContent = wrapWithNoActivityState(m_calendarWidget, &m_calendarTabStack);
    }

    {
        m_fitnessChart = new FitnessChartWidget(this);
        // Store wrapped fitness chart for FitnessPage.
        m_fitnessPageContent = wrapWithNoActivityState(m_fitnessChart, &m_fitnessTabStack);
    }

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

        // Override spinbox defaults with persisted settings (set in Settings dialog).
        {
            QSettings cs("Fitlyzer", "FitlyzerC");
            m_climbMinLengthSpin->setValue(   cs.value("climb/minLength",    0.15).toDouble());
            m_climbMinGainSpin->setValue(      cs.value("climb/minGain",     10.0).toDouble());
            m_climbMinGradientSpin->setValue(  cs.value("climb/minGradient",  4.0).toDouble());
            m_climbStartGradientSpin->setValue(cs.value("climb/startGrad",    1.5).toDouble());
            m_climbDipMetersSpin->setValue(    cs.value("climb/dipMeters",   10.0).toDouble());
            m_climbDipDistanceSpin->setValue(  cs.value("climb/dipDistance", 200.0).toDouble());
            m_climbSmoothingSpin->setValue(    cs.value("climb/smoothing",   50.0).toDouble());
        }

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
                if (!item)
                    continue;
                if (std::abs(item->data(kClimbStartRole).toDouble() - c.startSeconds) < 0.75 &&
                    std::abs(item->data(kClimbEndRole).toDouble() - c.endSeconds) < 0.75)
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

        // Wrap the climbing content in an empty-state guard for ClimbsPage.
        m_climbsPageContent = wrapWithNoActivityState(climbingTab, &m_climbingTabStack);
    }

    updateChartAnalysisEmptyStates();

    // ── Assemble the page stack ──────────────────────────────────────────────
    // Each entry at index N matches NavigationSidebar::Page enum value N.
    // Pages that are not yet fully migrated use PlaceholderPage.

    m_navigationSidebar = new NavigationSidebar(central);
    m_pageStack = new QStackedWidget(central);

    // [0] Activities
    m_activitiesPage = new ActivitiesPage(m_activityBrowser, m_pageStack);
    m_pageStack->addWidget(m_activitiesPage);

    // [1] Charts — ride charts, map, intervals, laps, and notes
    {
        // Assemble a header widget containing the Color By control and the
        // color-legend strip.  This becomes ChartsPage's top bar.
        auto* chartsColorBar = new QWidget;
        auto* chartsColorBarHL = new QHBoxLayout(chartsColorBar);
        chartsColorBarHL->setContentsMargins(0, 0, 0, 0);
        chartsColorBarHL->addLayout(colorBar);
        chartsColorBarHL->addWidget(m_colorLegendWidget);
        m_pageStack->addWidget(new ChartsPage(chartsColorBar, m_chartsPageContent, m_pageStack));
    }

    // [2] Power — Zones, Histogram, Power Curve (sub-tabbed)
    {
        m_analysisTabWidget = new QTabWidget(m_pageStack);
        m_analysisTabWidget->addTab(m_zonesPageContent,     "Zones");
        m_analysisTabWidget->addTab(m_histogramPageContent, "Histogram");
        m_analysisTabWidget->addTab(m_pdcPageContent,       "Power Curve");
        m_pageStack->addWidget(new PowerPage(m_analysisTabWidget, m_pageStack));
    }

    // [3] Intervals — dedicated page hosting interval controls and table.
    m_pageStack->addWidget(new IntervalsPage(m_intervalsPageContent, m_pageStack));

    // [4] Climbs — climb charts + climbs table
    {
        auto* climbsPage = new ClimbsPage(m_climbsPageContent, m_pageStack);
        connect(climbsPage, &ClimbsPage::addRequested, this, [this]()
        {
            if (m_climbAltitudeChart)
                m_climbAltitudeChart->setFocus();
            statusBar()->showMessage("Use the climb chart to create a new climb boundary.", 2500);
        });
        connect(climbsPage, &ClimbsPage::editRequested, this, &MainWindow::editSelectedClimbBoundaries);
        connect(climbsPage, &ClimbsPage::mergeRequested, this, &MainWindow::joinSelectedClimbs);
        connect(climbsPage, &ClimbsPage::deleteRequested, this, &MainWindow::removeSelectedClimbs);
        connect(climbsPage, &ClimbsPage::undoRequested, this, [this]()
        {
            if (m_undoManager)
                m_undoManager->undo();
        });
        connect(climbsPage, &ClimbsPage::redoRequested, this, [this]()
        {
            if (m_undoManager)
                m_undoManager->redo();
        });
        m_pageStack->addWidget(climbsPage);
    }

    // [5] Fitness — CTL/ATL/TSB and FTP history
    m_pageStack->addWidget(new FitnessPage(m_fitnessChart, m_pageStack));

    // [6] Calendar — activity and workout planning
    m_pageStack->addWidget(new CalendarPage(m_calendarWidget, m_pageStack));

    // [7] Video — video creation and export page
    {
        auto* videoPage = new VideoPage(m_pageStack);
        connect(videoPage, &VideoPage::previewRequested, this, &MainWindow::createVideo);
        connect(videoPage, &VideoPage::renderRequested, this, &MainWindow::createVideo);
        connect(videoPage, &VideoPage::exportRequested, this, &MainWindow::createVideo);
        m_pageStack->addWidget(videoPage);
    }

    // Sidebar (fixed width, left) | page stack (stretching, right)
    auto* contentRow = new QHBoxLayout;
    contentRow->setContentsMargins(0, 0, 0, 0);
    contentRow->setSpacing(0);
    contentRow->addWidget(m_navigationSidebar);
    contentRow->addWidget(m_pageStack, 1);
    mainLayout->addLayout(contentRow, 1);

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
