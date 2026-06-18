// SPDX-License-Identifier: GPL-3

#include "ChartController.h"

#include "../WorkoutController.h"
#include "../../analysis/PowerCurve.h"
#include "../../analysis/TrainingLoad.h"
#include "../../core/zones/ZoneCalculator.h"
#include "../../database/ActivityRepository.h"
#include "../../database/DatabaseManager.h"
#include "../../model/RideDataSerializer.h"
#include "charts/RideChartWidget.h"
#include "charts/PowerCurveWidget.h"
#include "charts/PowerHistogramWidget.h"
#include "charts/FitnessChartWidget.h"
#include "core/zones/ZoneDefinition.h"

#include <QDate>
#include <QTabWidget>
#include <QStackedLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QComboBox>
#include <QCheckBox>
#include <QLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QSizePolicy>

#include <map>
#include <cfloat>

namespace
{
QString fmtDur(double seconds)
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

QString rangeLabelForZone(const Zone& zone, ColorMetric metric)
{
    const QString unit = colorMetricUnit(metric);
    const QString minText = QString::number(zone.minValue, 'f', metric == ColorMetric::Speed ? 1 : 0);
    const QString maxText = QString::number(zone.maxValue, 'f', metric == ColorMetric::Speed ? 1 : 0);

    if (zone.maxValue >= DBL_MAX / 2.0)
        return QString("> %1 %2").arg(minText, unit).trimmed();

    return QString("%1 - %2 %3").arg(minText, maxText, unit).trimmed();
}
}

/**
 * @brief Constructs the chart controller.
 * @param controller Pointer to WorkoutController for data access.
 * @param dbManager Pointer to DatabaseManager for queries.
 * @param parent Parent object.
 */
ChartController::ChartController(
    WorkoutController* controller,
    DatabaseManager* dbManager,
    QObject* parent)
    : QObject(parent)
    , m_controller(controller)
    , m_dbManager(dbManager)
{
}

/**
 * @brief Destructor.
 */
ChartController::~ChartController() = default;

/**
 * @brief Sets the chart widgets managed by this controller.
 */
void ChartController::setChartWidgets(
    RideChartWidget* powerChart,
    RideChartWidget* hrChart,
    RideChartWidget* cadenceChart,
    RideChartWidget* speedChart,
    RideChartWidget* altitudeChart,
    PowerHistogramWidget* histogram,
    PowerCurveWidget* powerCurve,
    FitnessChartWidget* fitnessChart)
{
    m_powerChart = powerChart;
    m_hrChart = hrChart;
    m_cadenceChart = cadenceChart;
    m_speedChart = speedChart;
    m_altitudeChart = altitudeChart;
    m_histogram = histogram;
    m_powerCurve = powerCurve;
    m_fitnessChart = fitnessChart;

    connectChartSignals();
}

/**
 * @brief Sets the analysis tab-related widgets.
 */
void ChartController::setAnalysisTabWidgets(
    QTabWidget* analysisTabWidget,
    ColorLegendWidget* colorLegend,
    QStackedLayout* activityTabStack,
    QStackedLayout* zonesTabStack,
    QStackedLayout* histogramTabStack,
    QStackedLayout* powerCurveTabStack,
    QStackedLayout* calendarTabStack,
    QStackedLayout* fitnessTabStack)
{
    m_analysisTabWidget = analysisTabWidget;
    m_colorLegend = colorLegend;
    m_activityTabStack = activityTabStack;
    m_zonesTabStack = zonesTabStack;
    m_histogramTabStack = histogramTabStack;
    m_powerCurveTabStack = powerCurveTabStack;
    m_calendarTabStack = calendarTabStack;
    m_fitnessTabStack = fitnessTabStack;
}

/**
 * @brief Sets climbing tab stacked layout.
 */
void ChartController::setClimbingTabStack(QStackedLayout* climbingTabStack)
{
    m_climbingTabStack = climbingTabStack;
}

/**
 * @brief Sets the zones table widget for zone distribution display.
 */
void ChartController::setZonesTable(QTableWidget* zonesTable)
{
    m_zonesTable = zonesTable;
}

/**
 * @brief Sets the climb chart widgets.
 */
void ChartController::setClimbCharts(
    RideChartWidget* climbPowerChart,
    RideChartWidget* climbHrChart,
    RideChartWidget* climbCadenceChart,
    RideChartWidget* climbSpeedChart,
    RideChartWidget* climbAltitudeChart)
{
    m_climbPowerChart = climbPowerChart;
    m_climbHrChart = climbHrChart;
    m_climbCadenceChart = climbCadenceChart;
    m_climbSpeedChart = climbSpeedChart;
    m_climbAltitudeChart = climbAltitudeChart;
}

/**
 * @brief Sets the chart control widgets.
 */
void ChartController::setChartControls(
    QComboBox* colorMetricCombo,
    QComboBox* powerSmoothingCombo,
    QCheckBox* autoSmoothingCheck,
    QCheckBox* climbOverlayEnabledCheck,
    QComboBox* climbOverlayMetricCombo,
    QLayout* colorLegendLayout)
{
    m_colorMetricCombo = colorMetricCombo;
    m_powerSmoothingCombo = powerSmoothingCombo;
    m_autoSmoothingCheck = autoSmoothingCheck;
    m_climbOverlayEnabledCheck = climbOverlayEnabledCheck;
    m_climbOverlayMetricCombo = climbOverlayMetricCombo;
    m_colorLegendLayout = colorLegendLayout;
}

/**
 * @brief Sets athlete context for power curve queries.
 */
void ChartController::setAthleteId(int athleteId)
{
    m_currentAthleteId = athleteId;
}

/**
 * @brief Updates all managed charts with current activity data.
 */
void ChartController::updateCharts()
{
    if (!m_controller)
        return;

    const int colorMetricValue = m_colorMetricCombo ? m_colorMetricCombo->currentData().toInt() : 0;
    const ColorMetric colorMetric = static_cast<ColorMetric>(colorMetricValue);

    if (!m_powerChart)
        return;

    // Determine which climbs to show (filtered or all)
    const std::vector<Climb>& climbsForView = m_climbs.empty() ? m_controller->climbs() : m_climbs;

    // Apply smoothing settings first
    applyChartSmoothing();

    // Disable updates during population
    auto* parentWidget = qobject_cast<QWidget*>(parent());
    if (parentWidget)
        parentWidget->setUpdatesEnabled(false);

    // Update main ride charts
    m_powerChart->setIntervals(m_controller->intervals());
    m_powerChart->setClimbs(climbsForView);
    if (m_hrChart) m_hrChart->setClimbs(climbsForView);
    if (m_cadenceChart) m_cadenceChart->setClimbs(climbsForView);
    if (m_speedChart) m_speedChart->setClimbs(climbsForView);
    if (m_altitudeChart) m_altitudeChart->setClimbs(climbsForView);

    m_powerChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);
    if (m_hrChart) m_hrChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);
    if (m_cadenceChart) m_cadenceChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);
    if (m_speedChart) m_speedChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);
    if (m_altitudeChart) m_altitudeChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);

    // Update climb-specific charts if available
    if (m_climbPowerChart)
    {
        m_climbPowerChart->setClimbs(climbsForView);
        if (m_climbHrChart) m_climbHrChart->setClimbs(climbsForView);
        if (m_climbCadenceChart) m_climbCadenceChart->setClimbs(climbsForView);
        if (m_climbSpeedChart) m_climbSpeedChart->setClimbs(climbsForView);
        if (m_climbAltitudeChart) m_climbAltitudeChart->setClimbs(climbsForView);

        m_climbPowerChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);
        if (m_climbHrChart) m_climbHrChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);
        if (m_climbCadenceChart) m_climbCadenceChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);
        if (m_climbSpeedChart) m_climbSpeedChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);
        if (m_climbAltitudeChart) m_climbAltitudeChart->setRideData(m_controller->rideData(), colorMetric, m_colorContext);

        // Apply climb overlay if enabled
        if (m_climbAltitudeChart && m_climbOverlayEnabledCheck && m_climbOverlayMetricCombo)
        {
            const ColorMetric overlayMetric = static_cast<ColorMetric>(m_climbOverlayMetricCombo->currentData().toInt());
            m_climbAltitudeChart->setMetricOverlay(
                overlayMetric,
                m_climbOverlayEnabledCheck->isChecked());
        }
    }

    if (parentWidget)
        parentWidget->setUpdatesEnabled(true);

    emit chartsUpdated();
}

/**
 * @brief Refreshes chart display without reloading data.
 */
void ChartController::refreshCharts()
{
    // TODO: Extract chart refresh logic from MainWindow
    emit chartsUpdated();
}

/**
 * @brief Resets zoom on all charts to fit data.
 */
void ChartController::resetAllZoom()
{
    if (m_powerChart) m_powerChart->fitToData();
    if (m_hrChart) m_hrChart->fitToData();
    if (m_cadenceChart) m_cadenceChart->fitToData();
    if (m_speedChart) m_speedChart->fitToData();
    if (m_altitudeChart) m_altitudeChart->fitToData();
}

/**
 * @brief Updates power histogram display.
 */
void ChartController::updateHistogram()
{
    if (!m_histogram || !m_controller)
        return;

    m_histogram->setRideData(m_controller->rideData());
}

/**
 * @brief Updates fitness chart display.
 */
void ChartController::updateFitnessChart()
{
    if (!m_fitnessChart || !m_controller)
        return;

    if (!m_dbManager || !m_dbManager->isOpen() || m_currentAthleteId <= 0)
    {
        m_fitnessChart->clearChart();
        return;
    }

    auto db = m_dbManager->database();
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
        DailyLoadPoint point;
        auto it = tssPerDay.find(day);
        if (it != tssPerDay.end())
            point.tss = it->second;
        daily.push_back(point);
    }

    const std::vector<FitnessMetricsPoint> timeline = TrainingLoad::fitnessTimeline(daily);
    m_fitnessChart->setTimeline(timeline);
}

/**
 * @brief Updates empty state indicators for all chart tabs.
 */
void ChartController::updateAnalysisEmptyStates()
{
    if (!m_controller)
        return;

    const bool hasActivity = !m_controller->rideData().records.empty();
    const int page = hasActivity ? 1 : 0;

    const std::initializer_list<QStackedLayout*> stacks =
        { m_activityTabStack,
          m_zonesTabStack,
          m_histogramTabStack,
          m_powerCurveTabStack,
          m_calendarTabStack,
          m_fitnessTabStack,
          m_climbingTabStack };

    for (auto* stack : stacks)
    {
        if (stack)
            stack->setCurrentIndex(page);
    }
}

/**
 * @brief Synchronizes chart zoom levels across all ride charts.
 */
void ChartController::syncChartZoom()
{
    // TODO: Extract chart zoom sync from MainWindow
}

/**
 * @brief Synchronizes crosshair position across all ride charts.
 */
void ChartController::syncChartCrosshair()
{
    // TODO: Extract crosshair sync from MainWindow
}

/**
 * @brief Helper to connect chart widget signals for synchronization.
 */
void ChartController::connectChartSignals()
{
    // TODO: Extract signal connections from MainWindow buildUI
}

/**
 * @brief Helper to update color legend display.
 */
void ChartController::updateColorLegendDisplay()
{
    // TODO: Extract color legend update from MainWindow
}

/**
 * @brief Retrieves current color metric from stored value.
 */
int ChartController::currentColorMetric() const
{
    return m_colorMetric;
}

/**
 * @brief Sets intervals for display on power chart.
 */
void ChartController::setIntervals(const std::vector<Interval>& intervals)
{
    m_intervals = intervals;
}

/**
 * @brief Sets climbs for display on charts.
 */
void ChartController::setClimbs(const std::vector<Climb>& climbs)
{
    m_climbs = climbs;
}

/**
 * @brief Sets the color metric for chart visualization.
 */
void ChartController::setColorMetric(int colorMetric)
{
    m_colorMetric = colorMetric;
}

/**
 * @brief Sets the color context for chart rendering.
 */
void ChartController::setColorContext(const ColorContext& colorContext)
{
    m_colorContext = colorContext;
}

/**
 * @brief Updates zone availability display.
 */
void ChartController::updateZoneAvailability()
{
    if (!m_analysisTabWidget)
        return;

    const bool hasPower = m_controller && m_controller->statistics().maximumPower > 0.0;
    const ColorMetric metric = static_cast<ColorMetric>(m_colorMetric);
    const bool hasSelectedMetric = m_controller
        && ZoneCalculator::hasMetricSamples(m_controller->rideData(), metric);

    m_analysisTabWidget->setTabEnabled(1,
        metric != ColorMetric::None && hasSelectedMetric);
    m_analysisTabWidget->setTabEnabled(2, hasPower);
    m_analysisTabWidget->setTabEnabled(3, hasPower);
    m_analysisTabWidget->setTabEnabled(4, true);
    m_analysisTabWidget->setTabEnabled(5, true);
}

/**
 * @brief Runs full chart update sequence after workout load.
 */
void ChartController::handleWorkoutLoaded()
{
    const bool hasPower = m_controller && m_controller->statistics().maximumPower > 0.0;

    updateCharts();
    updateColorLegend();
    updateZoneAvailability();

    if (hasPower)
    {
        updateZonesTab();
        updateHistogram();
        updatePowerCurve();
        updateFitnessChart();
    }

    resetAllZoom();
}

/**
 * @brief Helper to apply current color metric to charts.
 */
void ChartController::applyColorMetric()
{
    // TODO: Extract color metric application from MainWindow
}

/**
 * @brief Updates zones table with zone distribution for current activity.
 */
void ChartController::updateZonesTab()
{
    if (!m_zonesTable || !m_controller)
        return;

    const ColorMetric metric = static_cast<ColorMetric>(m_colorMetric);
    const std::vector<ZoneDistribution> distributions = ZoneCalculator::computeDistribution(
        m_controller->rideData(),
        metric,
        m_colorContext);

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

/**
 * @brief Updates power curve display with athlete's historical power data.
 */
void ChartController::updatePowerCurve()
{
    if (!m_powerCurve || !m_controller)
        return;

    std::vector<PowerCurvePoint> last90;
    std::vector<PowerCurvePoint> currentYear;
    std::vector<PowerCurvePoint> allTime;

    if (m_dbManager && m_dbManager->isOpen() && m_currentAthleteId > 0)
    {
        auto db = m_dbManager->database();
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

                const RideData ride = RideDataSerializer::loadRideFromDatabase(activity.id, *m_dbManager);
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

    m_powerCurve->setRideDataWithComparisons(
        m_controller->rideData(),
        last90,
        currentYear,
        allTime,
        m_controller->ftp());
}

/**
 * @brief Updates color legend widget with current zone colors.
 */
void ChartController::updateColorLegend()
{
    if (!m_colorLegendLayout)
        return;

    QWidget* legendHost = m_colorLegendLayout->parentWidget();

    while (QLayoutItem* item = m_colorLegendLayout->takeAt(0))
    {
        if (QWidget* widget = item->widget())
            widget->deleteLater();
        delete item;
    }

    const ColorMetric metric = static_cast<ColorMetric>(m_colorMetric);
    if (metric == ColorMetric::None)
    {
        auto* label = new QLabel(QStringLiteral("Coloring disabled"), legendHost);
        label->setStyleSheet(QStringLiteral("color: #64748b;"));
        m_colorLegendLayout->addWidget(label);
        m_colorLegendLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
        return;
    }

    const std::vector<Zone> zones = ZoneCalculator::zonesForMetric(metric, m_colorContext);
    for (const Zone& zone : zones)
    {
        auto* swatch = new QLabel(legendHost);
        swatch->setFixedSize(12, 12);
        swatch->setStyleSheet(QString("background: %1; border: 1px solid rgba(15,23,42,0.15);")
                                  .arg(zone.color.name()));

        auto* text = new QLabel(zone.name, legendHost);

        auto* chip = new QWidget(legendHost);
        auto* chipLayout = new QHBoxLayout(chip);
        chipLayout->setContentsMargins(0, 0, 0, 0);
        chipLayout->setSpacing(4);
        chipLayout->addWidget(swatch);
        chipLayout->addWidget(text);
        m_colorLegendLayout->addWidget(chip);
    }

    m_colorLegendLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
}

/**
 * @brief Helper to apply smoothing settings to all charts.
 */
void ChartController::applyChartSmoothing()
{
    const std::initializer_list<RideChartWidget*> all =
        { m_powerChart, m_hrChart, m_cadenceChart, m_speedChart, m_altitudeChart,
          m_climbPowerChart, m_climbHrChart, m_climbCadenceChart, m_climbSpeedChart, m_climbAltitudeChart };

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
