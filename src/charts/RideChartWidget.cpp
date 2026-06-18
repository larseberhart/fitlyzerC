// SPDX-License-Identifier: GPL-3


#include "RideChartWidget.h"

#include <QColor>
#include <QCursor>
#include <QAction>
#include <QMenu>
#include <QPen>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QToolTip>
#include <QWheelEvent>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include "analysis/TrainingLoad.h"
#include "charts/data/RideChartDataBuilder.h"
#include "charts/rendering/RideChartRenderer.h"
#include "charts/selection/RideChartSelectionManager.h"
#include "core/zones/ColorProvider.h"
#include "core/zones/ZoneCalculator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace
{
}

using namespace RideChartDataBuilder;
using namespace RideChartSelectionManager;

using namespace RideChartRenderer;

RideChartWidget::RideChartWidget(Metric metric, QWidget* parent)
    : QChartView(new QChart, parent)
    , m_metric(metric)
{
    m_series = new QLineSeries;
    m_series->setUseOpenGL(false);
    m_series->setVisible(false);

    m_axisX = new QValueAxis;
    m_axisX->setVisible(false);

    m_axisXDisplay = new QCategoryAxis;
    m_axisXDisplay->setTitleText("Time");
    m_axisXDisplay->setGridLineVisible(true);
    m_axisXDisplay->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);

    m_axisY = new QValueAxis;
    m_axisY->setTitleText(labelForMetric(metric));
    m_axisY->setLabelFormat("%.0f");
    m_axisY->setGridLineVisible(true);

    m_axisYOverlay = new QValueAxis;
    m_axisYOverlay->setVisible(false);
    m_axisYOverlay->setGridLineVisible(false);
    m_axisYOverlay->setLabelsColor(QColor("#475569"));

    chart()->addSeries(m_series);
    chart()->addAxis(m_axisX,        Qt::AlignBottom);
    chart()->addAxis(m_axisXDisplay, Qt::AlignBottom);
    chart()->addAxis(m_axisY,        Qt::AlignLeft);
    chart()->addAxis(m_axisYOverlay, Qt::AlignRight);
    m_series->attachAxis(m_axisX);
    m_series->attachAxis(m_axisY);

    chart()->setTitle(labelForMetric(metric));
    chart()->legend()->hide();
    chart()->setMargins(QMargins(4, 4, 4, 4));
    chart()->setAnimationOptions(QChart::NoAnimation);

    // Horizontal-only rubber-band keeps Y-axis scale fixed
    setRubberBand(QChartView::HorizontalRubberBand);
    setRenderHint(QPainter::Antialiasing);

    // Mouse tracking for crosshair (no button press required)
    setMouseTracking(true);
    viewport()->setMouseTracking(true);

    // rangeChanged: clamp to [0, m_origMaxX], rebuild labels, emit for sync, rescale Y
    connect(m_axisX, &QValueAxis::rangeChanged,
        this, [this](double min, double max)
        {
            const double finalMin = std::max(0.0, min);
            const double finalMax = std::min(max, m_origMaxX);

            if (finalMin != min || finalMax != max)
            {
                QSignalBlocker blocker(m_axisX);
                m_axisX->setRange(finalMin, finalMax);
            }

            updateXTicks(finalMin, finalMax);
            updateYAxisForVisibleRange(finalMin, finalMax);

            emit visibleRangeChanged(finalMin, finalMax);

            if (!m_syncing)
                emit xRangeChanged(finalMin, finalMax);
        });

    setMinimumHeight(220);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

}

double RideChartWidget::visibleRangeStartMinutes() const
{
    return m_axisX ? m_axisX->min() : 0.0;
}

double RideChartWidget::visibleRangeEndMinutes() const
{
    return m_axisX ? m_axisX->max() : 0.0;
}

void RideChartWidget::setPowerSmoothingSeconds(int seconds)
{
    const int normalized = std::max(0, seconds);
    if (m_requestedPowerSmoothingSeconds == normalized)
        return;

    m_requestedPowerSmoothingSeconds = normalized;
    if (m_metric == Metric::Power)
        rebuildChart();
}

void RideChartWidget::setAutoSmoothingEnabled(bool enabled)
{
    if (m_autoSmoothing == enabled)
        return;

    m_autoSmoothing = enabled;
    if (m_metric == Metric::Power)
        rebuildChart();
}

void RideChartWidget::setIntervals(const std::vector<Interval>& intervals)
{
    m_intervals = intervals;
    if (m_metric == Metric::Power)
        rebuildChart();
}

void RideChartWidget::setClimbs(const std::vector<Climb>& climbs)
{
    m_climbs = climbs;
    rebuildChart();
}

void RideChartWidget::setMetricOverlay(ColorMetric metric, bool enabled)
{
    m_metricOverlayMetric = metric;
    m_metricOverlayEnabled = enabled && metric != ColorMetric::None;
    rebuildChart();
}

void RideChartWidget::setSelectedClimbIndex(int index)
{
    if (m_selectedClimbIndex == index)
        return;

    m_selectedClimbIndex = index;
    // Selected climb fill is rendered as a background band, so rebuild is needed
    // to reflect table single-click selection changes.
    rebuildChart();
}

void RideChartWidget::setClimbEditingEnabled(bool enabled)
{
    m_climbEditingEnabled = enabled;
    if (!enabled)
        m_climbHandleDrag = ClimbHandleDrag::None;
    scene()->invalidate(chart()->plotArea(), QGraphicsScene::ForegroundLayer);
}

std::vector<double> RideChartWidget::computeSmoothedPowerSeries(int seconds) const
{
    return computeSmoothedSeries(Metric::Power, seconds);
}

std::vector<double> RideChartWidget::computeSmoothedSeries(Metric metric, int seconds) const
{
    const int n = static_cast<int>(m_rideData.records.size());
    std::vector<double> out(static_cast<size_t>(n), std::numeric_limits<double>::quiet_NaN());
    if (n == 0)
        return out;

    // Check cache: key = (metric_id * 1000 + seconds)
    // Only cache if seconds > 0 (raw data doesn't need caching)
    if (seconds > 0)
    {
        const int metricId = static_cast<int>(metric);
        const int cacheKey = metricId * 1000 + seconds;
        auto cached = m_smoothedSeriesCache.find(cacheKey);
        if (cached != m_smoothedSeriesCache.end())
            return cached->second;
    }

    if (seconds <= 0)
    {
        for (int i = 0; i < n; ++i)
        {
            const RideRecord& record = m_rideData.records[static_cast<size_t>(i)];
            if (isValidSample(record, metric))
                out[static_cast<size_t>(i)] = valueForMetric(record, metric);
        }
        return out;
    }

    const double halfWindow = seconds / 2.0;
    std::vector<double> prefixSum(static_cast<size_t>(n) + 1, 0.0);
    std::vector<int> prefixCount(static_cast<size_t>(n) + 1, 0);

    for (int i = 0; i < n; ++i)
    {
        const RideRecord& record = m_rideData.records[static_cast<size_t>(i)];
        const bool valid = isValidSample(record, metric);
        prefixSum[static_cast<size_t>(i) + 1] = prefixSum[static_cast<size_t>(i)] + (valid ? valueForMetric(record, metric) : 0.0);
        prefixCount[static_cast<size_t>(i) + 1] = prefixCount[static_cast<size_t>(i)] + (valid ? 1 : 0);
    }

    int left = 0;
    int right = 0;
    for (int i = 0; i < n; ++i)
    {
        const RideRecord& record = m_rideData.records[static_cast<size_t>(i)];
        const double t = record.elapsedSeconds;

        while (left < n && m_rideData.records[static_cast<size_t>(left)].elapsedSeconds < t - halfWindow)
            ++left;
        while (right < n && m_rideData.records[static_cast<size_t>(right)].elapsedSeconds <= t + halfWindow)
            ++right;

        if (!isValidSample(record, metric))
            continue;

        const double sum = prefixSum[static_cast<size_t>(right)] - prefixSum[static_cast<size_t>(left)];
        const int count = prefixCount[static_cast<size_t>(right)] - prefixCount[static_cast<size_t>(left)];
        if (count > 0)
            out[static_cast<size_t>(i)] = sum / static_cast<double>(count);
    }

    // Store in cache
    const int metricId = static_cast<int>(metric);
    const int cacheKey = metricId * 1000 + seconds;
    m_smoothedSeriesCache[cacheKey] = out;

    return out;
}

std::vector<int> RideChartWidget::buildSampleIndices(const std::vector<double>& values,
                                                      int targetPointBudget) const
{
    std::vector<int> indices;
    const int total = static_cast<int>(values.size());
    if (total == 0)
        return indices;

    if (total <= targetPointBudget || targetPointBudget <= 8)
    {
        indices.reserve(static_cast<size_t>(total));
        for (int i = 0; i < total; ++i)
            indices.push_back(i);
        return indices;
    }

    const int bucketCount = std::max(1, targetPointBudget / 4);
    const double bucketSize = static_cast<double>(total) / static_cast<double>(bucketCount);
    indices.reserve(static_cast<size_t>(targetPointBudget));

    for (int bucket = 0; bucket < bucketCount; ++bucket)
    {
        const int begin = static_cast<int>(std::floor(bucket * bucketSize));
        const int end = std::min(total, static_cast<int>(std::floor((bucket + 1) * bucketSize)));
        if (begin >= end)
            continue;

        int first = -1;
        int last = -1;
        int minIdx = -1;
        int maxIdx = -1;
        double minVal = std::numeric_limits<double>::max();
        double maxVal = std::numeric_limits<double>::lowest();

        for (int i = begin; i < end; ++i)
        {
            const double v = values[static_cast<size_t>(i)];
            if (std::isnan(v))
                continue;

            if (first < 0)
                first = i;
            last = i;
            if (v < minVal)
            {
                minVal = v;
                minIdx = i;
            }
            if (v > maxVal)
            {
                maxVal = v;
                maxIdx = i;
            }
        }

        if (first < 0)
            continue;

        std::vector<int> bucketIndices = { first };
        if (minIdx >= 0 && minIdx != first)
            bucketIndices.push_back(minIdx);
        if (maxIdx >= 0 && maxIdx != first && maxIdx != minIdx)
            bucketIndices.push_back(maxIdx);
        if (last >= 0 && last != first && last != minIdx && last != maxIdx)
            bucketIndices.push_back(last);

        std::sort(bucketIndices.begin(), bucketIndices.end());
        for (int idx : bucketIndices)
        {
            if (indices.empty() || indices.back() != idx)
                indices.push_back(idx);
        }
    }

    if (!indices.empty() && indices.front() != 0)
        indices.insert(indices.begin(), 0);
    if (!indices.empty() && indices.back() != total - 1)
        indices.push_back(total - 1);
    if (indices.empty())
    {
        for (int i = 0; i < total; ++i)
            indices.push_back(i);
    }

    return indices;
}

void RideChartWidget::clearBackgroundSeries()
{
    for (QAreaSeries* series : m_backgroundSeries)
    {
        chart()->removeSeries(series);
        delete series;
    }
    m_backgroundSeries.clear();
}

void RideChartWidget::addZoneBackgroundBands(double minY, double maxY, double maxXMinutes)
{
    if (m_metric != Metric::Power)
        return;

    const std::vector<Zone> zones = ZoneCalculator::powerZones(m_colorContext.ftp);
    for (const Zone& zone : zones)
    {
        const double y0 = std::clamp(zone.minValue, minY, maxY);
        const double y1 = std::clamp(
            zone.maxValue >= std::numeric_limits<double>::max() / 2.0 ? maxY : zone.maxValue,
            minY,
            maxY);
        if (y1 <= y0)
            continue;

        auto* upper = new QLineSeries;
        auto* lower = new QLineSeries;
        upper->append(0.0, y1);
        upper->append(maxXMinutes, y1);
        lower->append(0.0, y0);
        lower->append(maxXMinutes, y0);

        auto* area = new QAreaSeries(upper, lower);
        QColor fill = zone.color;
        fill.setAlpha(24);
        area->setColor(fill);
        area->setPen(Qt::NoPen);

        chart()->addSeries(area);
        area->attachAxis(m_axisX);
        area->attachAxis(m_axisY);
        m_backgroundSeries.push_back(area);
    }
}

void RideChartWidget::addIntervalBackgroundBands(double minY, double maxY)
{
    if (m_metric != Metric::Power || m_intervals.empty())
        return;

    for (const Interval& iv : m_intervals)
    {
        const bool isWork = iv.label == "Work";
        const QColor base = isWork ? QColor("#dc2626") : QColor("#0284c7");

        auto* upper = new QLineSeries;
        auto* lower = new QLineSeries;
        const double x0 = iv.startSeconds / 60.0;
        const double x1 = iv.endSeconds / 60.0;
        upper->append(x0, maxY);
        upper->append(x1, maxY);
        lower->append(x0, minY);
        lower->append(x1, minY);

        auto* area = new QAreaSeries(upper, lower);
        QColor fill = base;
        fill.setAlpha(isWork ? 18 : 10);
        area->setColor(fill);
        area->setPen(Qt::NoPen);

        chart()->addSeries(area);
        area->attachAxis(m_axisX);
        area->attachAxis(m_axisY);
        m_backgroundSeries.push_back(area);
    }
}

void RideChartWidget::addMetricOverlayBands(double minY, double maxY)
{
    if (!m_metricOverlayEnabled || m_metricOverlayMetric == ColorMetric::None)
        return;

    const auto& records = m_rideData.records;
    if (records.size() < 2)
        return;

    auto quantizeColor = [](const QColor& in)
    {
        auto q = [](int c) { return (c / 32) * 32; };
        return QColor(q(in.red()), q(in.green()), q(in.blue()));
    };

    int segmentStart = -1;
    QColor current;

    auto flush = [&](int startIndex, int endIndex, const QColor& color)
    {
        if (startIndex < 0 || endIndex <= startIndex)
            return;

        const double x0 = records[static_cast<size_t>(startIndex)].elapsedSeconds / 60.0;
        const double x1 = records[static_cast<size_t>(endIndex)].elapsedSeconds / 60.0;
        if (x1 <= x0)
            return;

        auto* upper = new QLineSeries;
        auto* lower = new QLineSeries;
        upper->append(x0, maxY);
        upper->append(x1, maxY);
        lower->append(x0, minY);
        lower->append(x1, minY);

        auto* area = new QAreaSeries(upper, lower);
        QColor fill = color;
        fill.setAlpha(26);
        area->setColor(fill);
        area->setPen(Qt::NoPen);
        chart()->addSeries(area);
        area->attachAxis(m_axisX);
        area->attachAxis(m_axisY);
        m_backgroundSeries.push_back(area);
    };

    for (int i = 0; i < static_cast<int>(records.size()); ++i)
    {
        const RideRecord& r = records[static_cast<size_t>(i)];
        if (!hasColorMetricValue(m_metricOverlayMetric, r))
        {
            if (segmentStart >= 0)
            {
                flush(segmentStart, i, current);
                segmentStart = -1;
            }
            continue;
        }

        QColor c = quantizeColor(colorForMetricValue(m_metricOverlayMetric, m_colorContext, r));
        if (segmentStart < 0)
        {
            segmentStart = i;
            current = c;
            continue;
        }

        if (c != current)
        {
            flush(segmentStart, i, current);
            segmentStart = i;
            current = c;
        }
    }

    if (segmentStart >= 0)
        flush(segmentStart, static_cast<int>(records.size()) - 1, current);
}

void RideChartWidget::addClimbBackgroundBands(double minY, double maxY)
{
    if (m_climbs.empty())
        return;

    auto addBand = [this, minY, maxY](const Climb& climb, bool selected)
    {
        auto* upper = new QLineSeries;
        auto* lower = new QLineSeries;
        const double x0 = climb.startSeconds / 60.0;
        const double x1 = climb.endSeconds / 60.0;
        upper->append(x0, maxY);
        upper->append(x1, maxY);
        lower->append(x0, minY);
        lower->append(x1, minY);

        auto* area = new QAreaSeries(upper, lower);
        QColor fill("#16a34a");
        fill.setAlpha(selected ? 54 : 20);
        area->setColor(fill);
        if (selected)
        {
            QPen border(QColor("#15803d"));
            border.setWidthF(1.5);
            area->setPen(border);
        }
        else
        {
            area->setPen(Qt::NoPen);
        }

        chart()->addSeries(area);
        area->attachAxis(m_axisX);
        area->attachAxis(m_axisY);
        m_backgroundSeries.push_back(area);
    };

    // Draw non-selected climbs first, then selected climb on top.
    for (int idx = 0; idx < static_cast<int>(m_climbs.size()); ++idx)
    {
        if (idx == m_selectedClimbIndex)
            continue;
        addBand(m_climbs[static_cast<size_t>(idx)], false);
    }

    if (m_selectedClimbIndex >= 0 && m_selectedClimbIndex < static_cast<int>(m_climbs.size()))
        addBand(m_climbs[static_cast<size_t>(m_selectedClimbIndex)], true);
}

void RideChartWidget::updateXTicks(double minVal, double maxVal)
{
    const QStringList existing = m_axisXDisplay->categoriesLabels();
    for (const auto& lbl : existing)
        m_axisXDisplay->remove(lbl);

    const double span = maxVal - minVal;
    double interval;
    bool   showSeconds = false;

    if      (span <= 1.0)   { interval = 1.0 / 6.0; showSeconds = true; }
    else if (span <= 3.0)   { interval = 0.5;        showSeconds = true; }
    else if (span <= 8.0)   { interval = 1.0; }
    else if (span <= 20.0)  { interval = 2.0; }
    else if (span <= 40.0)  { interval = 5.0; }
    else if (span <= 90.0)  { interval = 10.0; }
    else if (span <= 180.0) { interval = 15.0; }
    else if (span <= 360.0) { interval = 30.0; }
    else                    { interval = 60.0; }

    auto formatTime = [showSeconds](double minutes) -> QString
    {
        const int totalSec = static_cast<int>(std::round(minutes * 60.0));
        const int h = totalSec / 3600;
        const int m = (totalSec % 3600) / 60;
        const int s = totalSec % 60;
        if (h > 0)
            return showSeconds
                ? QString("%1:%2:%3").arg(h).arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0'))
                : QString("%1:%2").arg(h).arg(m,2,10,QChar('0'));
        else
            return showSeconds
                ? QString("%1:%2").arg(m).arg(s,2,10,QChar('0'))
                : QString("%1").arg(m);
    };

    double firstTick = std::ceil(minVal / interval) * interval;
    if (firstTick < 0.0) firstTick = 0.0;

    for (double t = firstTick; t <= maxVal + interval * 1e-6; t += interval)
    {
        if (t < 0.0) continue;
        const QString label = formatTime(t);
        if (!m_axisXDisplay->categoriesLabels().contains(label))
            m_axisXDisplay->append(label, t);
    }

    m_axisXDisplay->setRange(minVal, maxVal);
}

void RideChartWidget::setRideData(const RideData& rideData,
                                  ColorMetric colorMetric,
                                  const ColorContext& colorContext)
{
    m_rideData = rideData;
    m_colorMetric = colorMetric;
    m_colorContext = colorContext;
    m_smoothedSeriesCache.clear();
    rebuildChart(false);
}

void RideChartWidget::rebuildChart(bool preserveXRange)
{
    if (m_isRebuilding)
        return;

    m_isRebuilding = true;
    
    // Invalidate Y-axis range cache since data is changing
    m_yAxisRangeCache.valid = false;

    const bool hadData = m_hasData;
    const double previousMinX = m_axisX->min();
    const double previousMaxX = m_axisX->max();

    for (QLineSeries* series : m_rawSeries)
    {
        chart()->removeSeries(series);
        delete series;
    }
    m_rawSeries.clear();

    for (QLineSeries* series : m_colorSeries)
    {
        chart()->removeSeries(series);
        delete series;
    }
    m_colorSeries.clear();

    for (QLineSeries* series : m_overlaySeries)
    {
        chart()->removeSeries(series);
        delete series;
    }
    m_overlaySeries.clear();

    for (QLineSeries* series : m_referenceSeries)
    {
        chart()->removeSeries(series);
        delete series;
    }
    m_referenceSeries.clear();
    clearBackgroundSeries();

    m_axisYOverlay->setVisible(false);
    m_axisYOverlay->setTitleText("");

    m_tooltipPoints.clear();
    m_tooltipRawValues.clear();
    m_tooltipDisplayValues.clear();

    if (m_rideData.records.empty())
    {
        clearChart();
        m_isRebuilding = false;
        return;
    }

    constexpr int kTargetPoints = 2600;
    const int total = static_cast<int>(m_rideData.records.size());
    const double visibleSpanMinutes = hadData ? std::max(0.0, previousMaxX - previousMinX) : m_rideData.records.back().elapsedSeconds / 60.0;
    m_effectivePowerSmoothingSeconds = m_autoSmoothing
        ? autoSmoothingSecondsForSpan(visibleSpanMinutes)
        : m_requestedPowerSmoothingSeconds;

    std::vector<double> primarySeries(total, std::numeric_limits<double>::quiet_NaN());
    if (m_effectivePowerSmoothingSeconds > 0)
    {
        // computeSmoothedSeries handles caching internally for all metrics
        primarySeries = computeSmoothedSeries(m_metric, m_effectivePowerSmoothingSeconds);
    }
    else
    {
        primarySeries = computeSmoothedSeries(m_metric, 0);
    }

    const std::vector<int> sampleIndices = buildSampleIndices(primarySeries, kTargetPoints);

    QList<QPointF> probePoints;
    probePoints.reserve(sampleIndices.size() + 2);

    double sum = 0.0;
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();
    int validCount = 0;

    QLineSeries* rawSeries = nullptr;
    QLineSeries* colorSeries = nullptr;
    QColor currentColor;
    bool hadValidPrimary = false;
    QPointF previousPrimaryPoint;

    auto makeRawSeries = [this]()
    {
        auto* series = new QLineSeries;
        QPen pen(QColor(37, 99, 235, 60));
        pen.setWidthF(1.0);
        series->setPen(pen);
        chart()->addSeries(series);
        series->attachAxis(m_axisX);
        series->attachAxis(m_axisY);
        m_rawSeries.push_back(series);
        return series;
    };

    auto makeColorSeries = [this](const QColor& color)
    {
        auto* series = new QLineSeries;
        QPen pen(color);
        pen.setWidthF(2.4);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        series->setPen(pen);
        chart()->addSeries(series);
        series->attachAxis(m_axisX);
        series->attachAxis(m_axisY);
        m_colorSeries.push_back(series);
        return series;
    };

    const bool showRawOverlay = (m_metric == Metric::Power && m_effectivePowerSmoothingSeconds > 0);

    for (int index : sampleIndices)
    {
        const RideRecord& record = m_rideData.records[static_cast<size_t>(index)];
        const bool rawValid = isValidSample(record, m_metric);
        const double rawValue = rawValid ? valueForMetric(record, m_metric) : std::numeric_limits<double>::quiet_NaN();

        const double primaryValue = primarySeries[static_cast<size_t>(index)];
        const bool primaryValid = !std::isnan(primaryValue);

        if (showRawOverlay)
        {
            if (rawValid)
            {
                if (!rawSeries)
                    rawSeries = makeRawSeries();
                rawSeries->append(record.elapsedSeconds / 60.0, rawValue);
            }
            else
            {
                rawSeries = nullptr;
            }
        }

        if (!primaryValid)
        {
            colorSeries = nullptr;
            hadValidPrimary = false;
            continue;
        }

        const QPointF point(record.elapsedSeconds / 60.0, primaryValue);
        probePoints.append(point);
        m_tooltipPoints.push_back(point);
        m_tooltipRawValues.push_back(rawValue);
        m_tooltipDisplayValues.push_back(primaryValue);
        sum += primaryValue;
        minY = std::min(minY, primaryValue);
        maxY = std::max(maxY, primaryValue);
        ++validCount;

        QColor edgeColor = QColor("#2563eb");
        if (m_colorMetric != ColorMetric::None && hasColorMetricValue(m_colorMetric, record))
            edgeColor = colorForMetricValue(m_colorMetric, m_colorContext, record);

        if (!hadValidPrimary)
        {
            colorSeries = makeColorSeries(edgeColor);
            colorSeries->append(point);
            currentColor = edgeColor;
        }
        else if (!colorSeries || currentColor != edgeColor)
        {
            colorSeries = makeColorSeries(edgeColor);
            colorSeries->append(previousPrimaryPoint);
            colorSeries->append(point);
            currentColor = edgeColor;
        }
        else
        {
            colorSeries->append(point);
        }

        previousPrimaryPoint = point;
        hadValidPrimary = true;
    }

    m_series->replace(probePoints);

    const double maxX = m_rideData.records.back().elapsedSeconds / 60.0;
    m_origMaxX = maxX;
    m_hasData = validCount > 0;

    if (m_hasData)
    {
        m_dataMinY = minY;
        m_dataMaxY = maxY;
        updateYAxis();

        addZoneBackgroundBands(m_dataMinY, m_dataMaxY, maxX);
        addMetricOverlayBands(m_dataMinY, m_dataMaxY);
        addIntervalBackgroundBands(m_dataMinY, m_dataMaxY);
        addClimbBackgroundBands(m_dataMinY, m_dataMaxY);

        if (m_metricOverlayEnabled && m_metricOverlayMetric != ColorMetric::None)
        {
            const Metric overlayMetric = metricFromColorMetric(m_metricOverlayMetric);
            std::vector<double> overlayValues = computeSmoothedSeries(overlayMetric, 0);
            const std::vector<int> overlayIndices = buildSampleIndices(overlayValues, kTargetPoints);

            double overlayMin = std::numeric_limits<double>::max();
            double overlayMax = std::numeric_limits<double>::lowest();
            QLineSeries* currentOverlay = nullptr;
            QColor currentOverlayColor;
            QPointF previousOverlayPoint;
            bool hadOverlayPoint = false;

            auto makeOverlaySeries = [this](const QColor& color)
            {
                auto* series = new QLineSeries;
                QPen pen(color);
                pen.setWidthF(1.8);
                pen.setCapStyle(Qt::RoundCap);
                pen.setJoinStyle(Qt::RoundJoin);
                series->setPen(pen);
                chart()->addSeries(series);
                series->attachAxis(m_axisX);
                series->attachAxis(m_axisYOverlay);
                m_overlaySeries.push_back(series);
                return series;
            };

            for (int idx : overlayIndices)
            {
                const RideRecord& r = m_rideData.records[static_cast<size_t>(idx)];
                if (!isValidSample(r, overlayMetric))
                {
                    currentOverlay = nullptr;
                    hadOverlayPoint = false;
                    continue;
                }

                const double y = overlayValues[static_cast<size_t>(idx)];
                if (std::isnan(y))
                {
                    currentOverlay = nullptr;
                    hadOverlayPoint = false;
                    continue;
                }

                overlayMin = std::min(overlayMin, y);
                overlayMax = std::max(overlayMax, y);

                const QPointF point(r.elapsedSeconds / 60.0, y);
                const QColor segColor = colorForMetricValue(m_metricOverlayMetric, m_colorContext, r);

                if (!currentOverlay)
                {
                    currentOverlay = makeOverlaySeries(segColor);
                    currentOverlayColor = segColor;
                    currentOverlay->append(point);
                }
                else if (currentOverlayColor != segColor)
                {
                    currentOverlay = makeOverlaySeries(segColor);
                    currentOverlayColor = segColor;
                    if (hadOverlayPoint)
                        currentOverlay->append(previousOverlayPoint);
                    currentOverlay->append(point);
                }
                else
                {
                    currentOverlay->append(point);
                }

                previousOverlayPoint = point;
                hadOverlayPoint = true;
            }

            if (!m_overlaySeries.empty())
            {
                // Keep metric overlay confined to the bottom third of the plot so
                // the altitude profile remains visually dominant.
                const double rawMin = overlayMin;
                const double rawMax = overlayMax;
                const double span = std::max(0.001, rawMax - rawMin);

                // Extend the axis maximum by 2× the data span so the overlay line
                // sits in the lower third, but anchor ticks at the actual data
                // range so labels don't appear in the empty upper portion.
                m_axisYOverlay->setRange(rawMin, rawMax + 2.0 * span);

                double niceMin = rawMin;
                double niceMax = rawMax;
                niceRange(rawMin, rawMax, niceMin, niceMax);
                const int desiredTicks = std::clamp(height() / 60, 3, 6);
                const double niceInterval = std::max(1.0, (niceMax - niceMin) / desiredTicks);
                m_axisYOverlay->setTickType(QValueAxis::TicksDynamic);
                m_axisYOverlay->setTickAnchor(niceMin);
                m_axisYOverlay->setTickInterval(niceInterval);

                m_axisYOverlay->setTitleText(labelForColorMetric(m_metricOverlayMetric));
                m_axisYOverlay->setVisible(true);
            }
        }

        if (m_metric == Metric::Power)
        {
            const double avg = sum / validCount;
            const double np = TrainingLoad::normalizedPower(m_rideData);
            auto addReferenceLine = [this, maxX](double y, const QColor& color, Qt::PenStyle style)
            {
                auto* series = new QLineSeries;
                QPen pen(color);
                pen.setWidthF(1.0);
                pen.setStyle(style);
                series->setPen(pen);
                series->append(0.0, y);
                series->append(maxX, y);
                chart()->addSeries(series);
                series->attachAxis(m_axisX);
                series->attachAxis(m_axisY);
                m_referenceSeries.push_back(series);
            };

            addReferenceLine(m_colorContext.ftp, QColor("#7c3aed"), Qt::DashLine);
            addReferenceLine(avg, QColor("#0f766e"), Qt::DotLine);
            if (np > 0.0)
                addReferenceLine(np, QColor("#b45309"), Qt::DashDotLine);

            // Mark best efforts by duration as vertical guide lines.
            const std::array<int, 9> bestDurations = { 1, 5, 15, 30, 60, 300, 600, 1200, 3600 };
            std::vector<double> rawPower(static_cast<size_t>(total), 0.0);
            std::vector<int> validMask(static_cast<size_t>(total), 0);
            for (int i = 0; i < total; ++i)
            {
                const RideRecord& rec = m_rideData.records[static_cast<size_t>(i)];
                if (rec.hasPower && rec.power > 0.0)
                {
                    rawPower[static_cast<size_t>(i)] = rec.power;
                    validMask[static_cast<size_t>(i)] = 1;
                }
            }

            std::vector<double> prefixSum(static_cast<size_t>(total) + 1, 0.0);
            std::vector<int> prefixCount(static_cast<size_t>(total) + 1, 0);
            for (int i = 0; i < total; ++i)
            {
                prefixSum[static_cast<size_t>(i) + 1] = prefixSum[static_cast<size_t>(i)] + rawPower[static_cast<size_t>(i)];
                prefixCount[static_cast<size_t>(i) + 1] = prefixCount[static_cast<size_t>(i)] + validMask[static_cast<size_t>(i)];
            }

            auto addBestMarker = [this](double xMinutes, double minV, double maxV, const QColor& color)
            {
                auto* series = new QLineSeries;
                QPen pen(color);
                pen.setWidthF(1.0);
                pen.setStyle(Qt::DashDotDotLine);
                series->setPen(pen);
                series->append(xMinutes, minV);
                series->append(xMinutes, maxV);
                chart()->addSeries(series);
                series->attachAxis(m_axisX);
                series->attachAxis(m_axisY);
                m_referenceSeries.push_back(series);
            };

            for (int duration : bestDurations)
            {
                if (duration <= 0 || duration >= total)
                    continue;

                double bestAvg = -1.0;
                int bestEndIndex = -1;
                for (int end = duration - 1; end < total; ++end)
                {
                    const int begin = end - duration + 1;
                    const double winSum = prefixSum[static_cast<size_t>(end) + 1] - prefixSum[static_cast<size_t>(begin)];
                    const int winCount = prefixCount[static_cast<size_t>(end) + 1] - prefixCount[static_cast<size_t>(begin)];
                    if (winCount <= 0)
                        continue;

                    const double avgPower = winSum / winCount;
                    if (avgPower > bestAvg)
                    {
                        bestAvg = avgPower;
                        bestEndIndex = end;
                    }
                }

                if (bestEndIndex < 0)
                    continue;

                const double xMinutes = m_rideData.records[static_cast<size_t>(bestEndIndex)].elapsedSeconds / 60.0;
                QColor markerColor("#1d4ed8");
                markerColor.setAlpha(170);
                addBestMarker(xMinutes, m_dataMinY, m_dataMaxY, markerColor);
            }

            const QString smoothingLabel = m_effectivePowerSmoothingSeconds <= 0
                ? QString("raw")
                : QString("%1s").arg(m_effectivePowerSmoothingSeconds);

            chart()->setTitle(
                QString("%1   min %2  avg %3  max %4  FTP %5  NP %6  smoothing %7")
                    .arg(labelForMetric(m_metric))
                    .arg(minY, 0, 'f', 0)
                    .arg(avg, 0, 'f', 0)
                    .arg(maxY, 0, 'f', 0)
                    .arg(m_colorContext.ftp)
                    .arg(np, 0, 'f', 0)
                    .arg(smoothingLabel));
        }
        else
        {
            const double avg = sum / validCount;
            chart()->setTitle(
                labelForMetric(m_metric) +
                QString("   min %1  avg %2  max %3  smoothing %4")
                    .arg(minY, 0, 'f', 0)
                    .arg(avg, 0, 'f', 0)
                    .arg(maxY, 0, 'f', 0)
                    .arg(m_effectivePowerSmoothingSeconds <= 0 ? QString("raw") : QString("%1s").arg(m_effectivePowerSmoothingSeconds)));
        }
    }
    else
    {
        m_axisY->setRange(0.0, 1.0);
        chart()->setTitle(labelForMetric(m_metric) + "   (no data)");
    }

    if (m_hasData)
    {
        if (preserveXRange && hadData && previousMaxX > previousMinX)
            m_axisX->setRange(std::max(0.0, previousMinX), std::min(previousMaxX, maxX));
        else
            m_axisX->setRange(0.0, maxX);
    }
    else
    {
        m_axisX->setRange(0.0, 1.0);
    }

    m_isRebuilding = false;
}

void RideChartWidget::updateYAxis()
{
    if (!m_hasData && m_dataMaxY <= m_dataMinY) return;

    double axisMin, axisMax;
    niceRange(m_dataMinY, m_dataMaxY, axisMin, axisMax);
    m_axisY->setRange(axisMin, axisMax);

    // Tick count: ~one label per 40 px of chart height, clamped 4-15.
    const int ticks = std::clamp(height() / 40, 4, 15);
    m_axisY->setTickCount(ticks);
}

void RideChartWidget::updateYAxisForVisibleRange(double xMinMin, double xMaxMin)
{
    if (!m_hasData || m_tooltipPoints.empty()) return;

    // Check cache: if same range, reuse calculation
    if (m_yAxisRangeCache.valid &&
        xMinMin == m_yAxisRangeCache.cachedMinX &&
        xMaxMin == m_yAxisRangeCache.cachedMaxX)
    {
        m_axisY->setRange(m_yAxisRangeCache.cachedAxisMin, m_yAxisRangeCache.cachedAxisMax);
        return;
    }

    double visMin = std::numeric_limits<double>::max();
    double visMax = std::numeric_limits<double>::lowest();

    for (const QPointF& pt : m_tooltipPoints)
    {
        if (pt.x() >= xMinMin && pt.x() <= xMaxMin)
        {
            visMin = std::min(visMin, pt.y());
            visMax = std::max(visMax, pt.y());
        }
    }

    if (visMin > visMax)
    {
        // No points in view — fall back to full range
        updateYAxis();
        return;
    }

    double axisMin, axisMax;
    niceRange(visMin, visMax, axisMin, axisMax);
    
    // Cache this result for rapid zoom operations
    m_yAxisRangeCache.cachedMinX = xMinMin;
    m_yAxisRangeCache.cachedMaxX = xMaxMin;
    m_yAxisRangeCache.cachedAxisMin = axisMin;
    m_yAxisRangeCache.cachedAxisMax = axisMax;
    m_yAxisRangeCache.valid = true;
    
    m_axisY->setRange(axisMin, axisMax);

    const int ticks = std::clamp(height() / 40, 4, 15);
    m_axisY->setTickCount(ticks);
}

void RideChartWidget::resizeEvent(QResizeEvent* event)
{
    QChartView::resizeEvent(event);
    // Only update tick count when we have data; don't change range on resize.
    if (m_hasData)
    {
        const int ticks = std::clamp(height() / 40, 4, 15);
        m_axisY->setTickCount(ticks);
    }
}

void RideChartWidget::clearChart()
{
    for (QLineSeries* series : m_rawSeries)
    {
        chart()->removeSeries(series);
        delete series;
    }
    m_rawSeries.clear();

    for (QLineSeries* series : m_colorSeries)
    {
        chart()->removeSeries(series);
        delete series;
    }
    m_colorSeries.clear();

    for (QLineSeries* series : m_overlaySeries)
    {
        chart()->removeSeries(series);
        delete series;
    }
    m_overlaySeries.clear();

    for (QLineSeries* series : m_referenceSeries)
    {
        chart()->removeSeries(series);
        delete series;
    }
    m_referenceSeries.clear();
    clearBackgroundSeries();
    m_series->clear();
    m_smoothedSeriesCache.clear();
    m_yAxisRangeCache.valid = false;
    m_tooltipPoints.clear();
    m_tooltipRawValues.clear();
    m_tooltipDisplayValues.clear();
    m_hasData    = false;
    m_origMaxX   = 1.0;
    m_crosshairX = -1.0;
    updateXTicks(0.0, 1.0);
    m_axisX->setRange(0.0, 1.0);
    m_axisY->setRange(0.0, 1.0);
    m_axisYOverlay->setVisible(false);
    m_axisYOverlay->setTitleText("");
    chart()->setTitle(labelForMetric(m_metric));
}

void RideChartWidget::setXRange(double min, double max)
{
    m_syncing = true;
    m_axisX->setRange(min, max);
    m_syncing = false;
    emit visibleRangeChanged(m_axisX->min(), m_axisX->max());
}

void RideChartWidget::setCrosshair(double xMinutes)
{
    m_crosshairX = xMinutes;

    if (m_crosshairX < 0.0 || m_tooltipPoints.empty())
    {
        m_crosshairY = std::numeric_limits<double>::quiet_NaN();
        scene()->invalidate(chart()->plotArea(), QGraphicsScene::ForegroundLayer);
        return;
    }

    const auto it = std::lower_bound(
        m_tooltipPoints.begin(),
        m_tooltipPoints.end(),
        m_crosshairX,
        [](const QPointF& point, double x) { return point.x() < x; });

    size_t index = 0;
    if (it == m_tooltipPoints.end())
        index = m_tooltipPoints.size() - 1;
    else
        index = static_cast<size_t>(std::distance(m_tooltipPoints.begin(), it));

    if (index > 0 && index < m_tooltipPoints.size())
    {
        const double currDist = std::abs(m_tooltipPoints[index].x() - m_crosshairX);
        const double prevDist = std::abs(m_tooltipPoints[index - 1].x() - m_crosshairX);
        if (prevDist < currDist)
            --index;
    }

    m_crosshairY = m_tooltipPoints[index].y();
    scene()->invalidate(chart()->plotArea(), QGraphicsScene::ForegroundLayer);
}

void RideChartWidget::resetZoom()
{
    if (m_hasData)
    {
        m_axisX->setRange(0.0, m_origMaxX);
        emit visibleRangeChanged(m_axisX->min(), m_axisX->max());
    }
}

void RideChartWidget::fitToData()
{
    resetZoom();
}

void RideChartWidget::wheelEvent(QWheelEvent* event)
{
    if (m_newClimbCreating)
    {
        event->accept();
        return;
    }

    if (m_climbEditingEnabled &&
        m_selectedClimbIndex >= 0 &&
        m_selectedClimbIndex < static_cast<int>(m_climbs.size()))
    {
        const Climb& climb = m_climbs[static_cast<size_t>(m_selectedClimbIndex)];
        const QPointF scenePos = mapToScene(event->position().toPoint());
        if (hitTestClimbHandles(chart(), m_series, m_axisY, climb, scenePos).hit)
        {
            event->accept();
            return;
        }
    }

    const double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;

    const double lo     = m_axisX->min();
    const double hi     = m_axisX->max();
    const double center = (lo + hi) / 2.0;
    const double half   = (hi - lo) / 2.0 / factor;

    const double newMin = std::max(0.0,        center - half);
    const double newMax = std::min(m_origMaxX, center + half);

    if (newMax > newMin)
    {
        m_axisX->setRange(newMin, newMax);
        emit visibleRangeChanged(m_axisX->min(), m_axisX->max());
    }

    event->accept();
}

void RideChartWidget::mousePressEvent(QMouseEvent* event)
{
    if (m_newClimbCreating && m_hasData && event->button() == Qt::LeftButton)
    {
        // Update current end position on press; mouseRelease will commit.
        const QPointF scenePos = mapToScene(event->pos());
        const QRectF plotArea = chart()->plotArea();
        const QPointF clampedPos(
            std::clamp(scenePos.x(), plotArea.left(), plotArea.right()),
            std::clamp(scenePos.y(), plotArea.top(), plotArea.bottom()));
        const double xMinutes = chart()->mapToValue(clampedPos, m_series).x();
        m_newClimbCurrentSec = std::clamp(xMinutes * 60.0, 0.0, m_origMaxX * 60.0);
        scene()->invalidate(chart()->plotArea(), QGraphicsScene::ForegroundLayer);
        event->accept();
        return;
    }

    if (m_climbEditingEnabled &&
        m_hasData &&
        event->button() == Qt::RightButton)
    {
        const QPointF scenePos = mapToScene(event->pos());
        if (chart()->plotArea().contains(scenePos))
        {
            const double xMinutes = chart()->mapToValue(scenePos, m_series).x();
            const double anchorSec = std::clamp(xMinutes * 60.0, 0.0, m_origMaxX * 60.0);

            QMenu menu(this);
            QAction* newClimbAction = menu.addAction("New Climb");
            QAction* selected = menu.exec(event->globalPosition().toPoint());
            if (selected == newClimbAction)
            {
                m_newClimbCreating = true;
                m_newClimbDragging = true;  // box visible immediately after menu close
                m_newClimbStartSec = anchorSec;
                m_newClimbCurrentSec = anchorSec;
                viewport()->setCursor(Qt::SizeHorCursor);
                setMouseTracking(true);
                scene()->invalidate(chart()->plotArea(), QGraphicsScene::ForegroundLayer);
                event->accept();
                return;
            }
        }
    }

    if (m_climbEditingEnabled &&
        m_hasData &&
        event->button() == Qt::LeftButton &&
        m_selectedClimbIndex >= 0 &&
        m_selectedClimbIndex < static_cast<int>(m_climbs.size()))
    {
        const Climb& climb = m_climbs[static_cast<size_t>(m_selectedClimbIndex)];
        const QPointF scenePos = mapToScene(event->pos());
        const ClimbHandleHitResult hit = hitTestClimbHandles(chart(), m_series, m_axisY, climb, scenePos);
        if (hit.hit)
        {
            m_climbHandleDrag = hit.startHandle ? ClimbHandleDrag::Start : ClimbHandleDrag::End;
            m_dragOriginalClimbStartSec = climb.startSeconds;
            m_dragOriginalClimbEndSec = climb.endSeconds;
            viewport()->setCursor(Qt::SizeHorCursor);
            event->accept();
            return;
        }
    }

    // Left-click inside a climb band → select it
    if (m_hasData && !m_climbs.empty() && event->button() == Qt::LeftButton)
    {
        const QPointF scenePos = mapToScene(event->pos());
        if (chart()->plotArea().contains(scenePos))
        {
            const double xSec = chart()->mapToValue(scenePos, m_series).x() * 60.0;
            for (int i = 0; i < static_cast<int>(m_climbs.size()); ++i)
            {
                const Climb& c = m_climbs[static_cast<size_t>(i)];
                if (xSec >= c.startSeconds && xSec <= c.endSeconds)
                {
                    emit climbClicked(i);
                    event->accept();
                    return;
                }
            }
        }
    }

    if (m_metric == Metric::Power &&
        m_hasData &&
        event->button() == Qt::RightButton &&
        (event->modifiers() & Qt::ShiftModifier))
    {
        const QPointF scenePos = mapToScene(event->pos());
        if (chart()->plotArea().contains(scenePos))
        {
            const double x = chart()->mapToValue(scenePos, m_series).x();
            m_intervalSelecting = true;
            m_intervalStartMin = std::clamp(x, 0.0, m_origMaxX);
            m_intervalEndMin = m_intervalStartMin;
            scene()->invalidate(chart()->plotArea(), QGraphicsScene::ForegroundLayer);
            event->accept();
            return;
        }
    }

    if (event->button() == Qt::RightButton)
    {
        event->accept();
        return;
    }

    QChartView::mousePressEvent(event);
}

void RideChartWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_newClimbCreating && event->button() == Qt::LeftButton)
    {
        if (m_newClimbDragging)
        {
            const double startSec = std::min(m_newClimbStartSec, m_newClimbCurrentSec);
            const double endSec = std::max(m_newClimbStartSec, m_newClimbCurrentSec);
            if (endSec - startSec >= 1.0)
                emit newClimbRequested(startSec, endSec);
        }

        m_newClimbCreating = false;
        m_newClimbDragging = false;
        m_newClimbStartSec = -1.0;
        m_newClimbCurrentSec = -1.0;
        viewport()->unsetCursor();
        scene()->invalidate(chart()->plotArea(), QGraphicsScene::ForegroundLayer);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && m_climbHandleDrag != ClimbHandleDrag::None)
    {
        if (m_selectedClimbIndex >= 0 &&
            m_selectedClimbIndex < static_cast<int>(m_climbs.size()))
        {
            const Climb& climb = m_climbs[static_cast<size_t>(m_selectedClimbIndex)];
            if (std::abs(climb.startSeconds - m_dragOriginalClimbStartSec) > 1e-3 ||
                std::abs(climb.endSeconds - m_dragOriginalClimbEndSec) > 1e-3)
            {
                emit climbBoundaryEdited(
                    m_dragOriginalClimbStartSec,
                    m_dragOriginalClimbEndSec,
                    climb.startSeconds,
                    climb.endSeconds);
            }
        }

        m_climbHandleDrag = ClimbHandleDrag::None;
        viewport()->unsetCursor();
        event->accept();
        return;
    }

    if (m_intervalSelecting && event->button() == Qt::RightButton)
    {
        m_intervalSelecting = false;
        const double startMin = std::min(m_intervalStartMin, m_intervalEndMin);
        const double endMin = std::max(m_intervalStartMin, m_intervalEndMin);
        m_intervalStartMin = -1.0;
        m_intervalEndMin = -1.0;
        scene()->invalidate(chart()->plotArea(), QGraphicsScene::ForegroundLayer);

        const double startSec = startMin * 60.0;
        const double endSec = endMin * 60.0;
        if (endSec - startSec >= 1.0)
            emit intervalSelectionRequested(startSec, endSec);

        event->accept();
        return;
    }

    if (event->button() == Qt::RightButton)
    {
        event->accept();
        return;
    }

    QChartView::mouseReleaseEvent(event);
}

void RideChartWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        event->accept();
        return;
    }

    if (m_newClimbCreating)
    {
        event->accept();
        return;
    }

    if (m_climbEditingEnabled &&
        m_selectedClimbIndex >= 0 &&
        m_selectedClimbIndex < static_cast<int>(m_climbs.size()))
    {
        const Climb& climb = m_climbs[static_cast<size_t>(m_selectedClimbIndex)];
        const QPointF scenePos = mapToScene(event->pos());
        if (hitTestClimbHandles(chart(), m_series, m_axisY, climb, scenePos).hit)
        {
            event->accept();
            return;
        }
    }

    if (m_hasData)
    {
        m_axisX->setRange(0.0, m_origMaxX);
        emit visibleRangeChanged(m_axisX->min(), m_axisX->max());
    }
    QChartView::mouseDoubleClickEvent(event);
}

void RideChartWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_newClimbCreating)
    {
        const QPointF scenePos = mapToScene(event->pos());
        const QRectF plotArea = chart()->plotArea();
        const QPointF clampedChartPos(
            std::clamp(scenePos.x(), plotArea.left(), plotArea.right()),
            std::clamp(scenePos.y(), plotArea.top(), plotArea.bottom()));
        const double xMinutes = chart()->mapToValue(clampedChartPos, m_series).x();
        m_newClimbCurrentSec = std::clamp(xMinutes * 60.0, 0.0, m_origMaxX * 60.0);
        scene()->invalidate(chart()->plotArea(), QGraphicsScene::ForegroundLayer);
        event->accept();
        return;
    }

    if (m_climbHandleDrag != ClimbHandleDrag::None &&
        m_selectedClimbIndex >= 0 &&
        m_selectedClimbIndex < static_cast<int>(m_climbs.size()))
    {
        const QPointF chartPos = mapToScene(event->pos());
        const QRectF plotArea = chart()->plotArea();
        const QPointF clampedChartPos(
            std::clamp(chartPos.x(), plotArea.left(), plotArea.right()),
            std::clamp(chartPos.y(), plotArea.top(), plotArea.bottom()));
        const double xMinutes = chart()->mapToValue(clampedChartPos, m_series).x();
        const double xSec = std::clamp(xMinutes * 60.0, 0.0, m_origMaxX * 60.0);

        Climb& climb = m_climbs[static_cast<size_t>(m_selectedClimbIndex)];
        const double oldStart = climb.startSeconds;
        const double oldEnd = climb.endSeconds;
        constexpr double kMinClimbDurationSec = 1.0;

        if (m_climbHandleDrag == ClimbHandleDrag::Start)
            climb.startSeconds = std::min(xSec, climb.endSeconds - kMinClimbDurationSec);
        else
            climb.endSeconds = std::max(xSec, climb.startSeconds + kMinClimbDurationSec);

        (void)oldStart;
        (void)oldEnd;
        scene()->invalidate(chart()->plotArea(), QGraphicsScene::ForegroundLayer);
        event->accept();
        return;
    }

    if (m_climbEditingEnabled &&
        m_selectedClimbIndex >= 0 &&
        m_selectedClimbIndex < static_cast<int>(m_climbs.size()))
    {
        const Climb& climb = m_climbs[static_cast<size_t>(m_selectedClimbIndex)];
        const QPointF scenePos = mapToScene(event->pos());
        const bool hoverHandle = hitTestClimbHandles(chart(), m_series, m_axisY, climb, scenePos).hit;
        if (hoverHandle)
        {
            viewport()->setCursor(Qt::SizeHorCursor);
            event->accept();
            return;
        }
        viewport()->unsetCursor();
    }

    if (m_hasData)
    {
        const QPointF scenePos = mapToScene(event->pos());
        if (chart()->plotArea().contains(scenePos))
        {
            const double dataX = chart()->mapToValue(scenePos, m_series).x();
            m_crosshairX = std::clamp(dataX, 0.0, m_origMaxX);
            m_crosshairY = chart()->mapToValue(scenePos, m_series).y();

            if (m_intervalSelecting)
            {
                m_intervalEndMin = m_crosshairX;
                scene()->invalidate(chart()->plotArea(), QGraphicsScene::ForegroundLayer);
                event->accept();
                return;
            }

            emit crosshairMoved(m_crosshairX);
            scene()->invalidate(chart()->plotArea(), QGraphicsScene::ForegroundLayer);

            if (!m_tooltipPoints.empty())
            {
                const auto it = std::lower_bound(
                    m_tooltipPoints.begin(),
                    m_tooltipPoints.end(),
                    m_crosshairX,
                    [](const QPointF& point, double x) { return point.x() < x; });

                size_t index = 0;
                if (it == m_tooltipPoints.end())
                    index = m_tooltipPoints.size() - 1;
                else
                    index = static_cast<size_t>(std::distance(m_tooltipPoints.begin(), it));

                if (index > 0 && index < m_tooltipPoints.size())
                {
                    const double currDist = std::abs(m_tooltipPoints[index].x() - m_crosshairX);
                    const double prevDist = std::abs(m_tooltipPoints[index - 1].x() - m_crosshairX);
                    if (prevDist < currDist)
                        --index;
                }

                const QPointF& point = m_tooltipPoints[index];
                m_crosshairY = point.y();
                const int totalSec = static_cast<int>(point.x() * 60.0);
                const int h = totalSec / 3600;
                const int m = (totalSec % 3600) / 60;
                const int s = totalSec % 60;
                const double visibleMinutes = m_axisX->max() - m_axisX->min();
                const bool showSeconds = visibleMinutes <= 5.0;

                QString timeStr;
                if (h > 0)
                    timeStr = showSeconds
                        ? QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'))
                        : QString("%1:%2").arg(h).arg(m, 2, 10, QChar('0'));
                else
                    timeStr = showSeconds
                        ? QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0'))
                        : QString("%1 min").arg(m);

                size_t nearestRecordIndex = 0;
                if (!m_rideData.records.empty())
                {
                    const auto rit = std::lower_bound(
                        m_rideData.records.begin(),
                        m_rideData.records.end(),
                        point.x() * 60.0,
                        [](const RideRecord& rec, double sec) { return rec.elapsedSeconds < sec; });
                    if (rit == m_rideData.records.end())
                        nearestRecordIndex = m_rideData.records.size() - 1;
                    else
                        nearestRecordIndex = static_cast<size_t>(std::distance(m_rideData.records.begin(), rit));
                    if (nearestRecordIndex > 0 && nearestRecordIndex < m_rideData.records.size())
                    {
                        const double currDist = std::abs(m_rideData.records[nearestRecordIndex].elapsedSeconds - point.x() * 60.0);
                        const double prevDist = std::abs(m_rideData.records[nearestRecordIndex - 1].elapsedSeconds - point.x() * 60.0);
                        if (prevDist < currDist)
                            --nearestRecordIndex;
                    }
                }

                const RideRecord& rec = m_rideData.records[nearestRecordIndex];

                double rolling30Power = 0.0;
                int rollingCount = 0;
                const size_t startIdx = nearestRecordIndex > 30 ? (nearestRecordIndex - 30) : 0;
                for (size_t i = startIdx; i <= nearestRecordIndex; ++i)
                {
                    const RideRecord& rp = m_rideData.records[i];
                    if (rp.hasPower && rp.power > 0.0)
                    {
                        rolling30Power += rp.power;
                        ++rollingCount;
                    }
                }
                if (rollingCount > 0)
                    rolling30Power /= static_cast<double>(rollingCount);

                QColor zoneColor = ColorProvider::colorForPower(rec.power, m_colorContext.ftp);
                const std::vector<Zone> powerZones = ZoneCalculator::powerZones(m_colorContext.ftp);
                QString zoneLabel = "-";
                for (size_t zi = 0; zi < powerZones.size(); ++zi)
                {
                    const Zone& z = powerZones[zi];
                    if (rec.power >= z.minValue && rec.power < z.maxValue)
                    {
                        zoneLabel = QString("Z%1 %2").arg(static_cast<int>(zi) + 1).arg(z.name);
                        break;
                    }
                }

                QString tooltip;
                tooltip += QString("Time: %1").arg(timeStr);
                tooltip += QString("\nPower: %1 W").arg(rec.hasPower ? QString::number(rec.power, 'f', 0) : "-");
                tooltip += QString("\n30s Power: %1 W").arg(rollingCount > 0 ? QString::number(rolling30Power, 'f', 0) : "-");
                tooltip += QString("\nHR: %1 bpm").arg(rec.hasHeartRate ? QString::number(rec.heartRate, 'f', 0) : "-");
                tooltip += QString("\nCadence: %1 rpm").arg(rec.hasCadence ? QString::number(rec.cadence, 'f', 0) : "-");
                tooltip += QString("\nSpeed: %1 km/h").arg(rec.hasSpeed ? QString::number(rec.speed, 'f', 1) : "-");
                tooltip += QString("\nAltitude: %1 m").arg(rec.hasAltitude ? QString::number(rec.altitude, 'f', 0) : "-");
                tooltip += QString("\nZone: %1").arg(zoneLabel);
                tooltip += QString("\nColor: %1").arg(zoneColor.name());

                if (m_metric == Metric::Power && m_effectivePowerSmoothingSeconds > 0)
                    tooltip += QString("\nSmoothed: %1 W").arg(m_tooltipDisplayValues[index], 0, 'f', 0);

                QToolTip::showText(QCursor::pos(), tooltip, this);
            }
        }
    }
    QChartView::mouseMoveEvent(event);
}

void RideChartWidget::leaveEvent(QEvent* event)
{
    if (m_newClimbCreating && !m_newClimbDragging)
    {
        m_newClimbCreating = false;
        m_newClimbStartSec = -1.0;
        m_newClimbCurrentSec = -1.0;
    }

    viewport()->unsetCursor();
    m_crosshairX = -1.0;
    m_crosshairY = std::numeric_limits<double>::quiet_NaN();
    QToolTip::hideText();
    emit crosshairMoved(-1.0);
    scene()->invalidate(chart()->plotArea(), QGraphicsScene::ForegroundLayer);
    QChartView::leaveEvent(event);
}

void RideChartWidget::drawForeground(QPainter* painter, const QRectF&)
{
    if (!m_hasData) return;

    const QRectF plotArea = chart()->plotArea();
    painter->save();

    // --- New-climb creation overlay (line → box) ---------------------------
    if (m_newClimbCreating && m_newClimbStartSec >= 0.0)
    {
        const double x0sec = std::min(m_newClimbStartSec, m_newClimbCurrentSec);
        const double x1sec = std::max(m_newClimbStartSec, m_newClimbCurrentSec);
        const QPointF p0 = chart()->mapToPosition(QPointF(x0sec / 60.0, m_axisY->min()), m_series);
        const QPointF p1 = chart()->mapToPosition(QPointF(x1sec / 60.0, m_axisY->min()), m_series);
        const double px0 = std::clamp(p0.x(), plotArea.left(), plotArea.right());
        const double px1 = std::clamp(p1.x(), plotArea.left(), plotArea.right());

        painter->setPen(QPen(QColor("#16a34a"), 2.0, Qt::SolidLine));

        constexpr double kBoxThreshold = 4.0; // px — show box once wider than this
        if (px1 - px0 < kBoxThreshold)
        {
            // Just a vertical line at anchor
            painter->drawLine(QLineF(px0, plotArea.top(), px0, plotArea.bottom()));
        }
        else
        {
            QColor fill("#16a34a");
            fill.setAlpha(45);
            const QRectF box(QPointF(px0, plotArea.top()), QPointF(px1, plotArea.bottom()));
            painter->fillRect(box, fill);
            painter->drawRect(box);
        }

        painter->restore();
        return; // skip crosshair while creating
    }
    // -----------------------------------------------------------------------

    if (m_crosshairX < 0.0)
    {
        painter->restore();
        return;
    }

    const QPointF chartPos =
        chart()->mapToPosition(QPointF(m_crosshairX, m_axisY->min()), m_series);

    if (chartPos.x() < plotArea.left() || chartPos.x() > plotArea.right())
    {
        painter->restore();
        return;
    }

    painter->setPen(QPen(QColor(220, 220, 220, 200), 1, Qt::DashLine));
    painter->drawLine(QLineF(chartPos.x(), plotArea.top(),
                             chartPos.x(), plotArea.bottom()));

    if (!std::isnan(m_crosshairY))
    {
        const QPointF yPos = chart()->mapToPosition(QPointF(m_axisX->min(), m_crosshairY), m_series);
        if (yPos.y() >= plotArea.top() && yPos.y() <= plotArea.bottom())
            painter->drawLine(QLineF(plotArea.left(), yPos.y(), plotArea.right(), yPos.y()));
    }

    if (m_intervalSelecting && m_intervalStartMin >= 0.0 && m_intervalEndMin >= 0.0)
    {
        const double x0 = std::min(m_intervalStartMin, m_intervalEndMin);
        const double x1 = std::max(m_intervalStartMin, m_intervalEndMin);
        const QPointF p0 = chart()->mapToPosition(QPointF(x0, m_axisY->min()), m_series);
        const QPointF p1 = chart()->mapToPosition(QPointF(x1, m_axisY->min()), m_series);
        const QRectF selectionRect(
            QPointF(std::max(plotArea.left(), p0.x()), plotArea.top()),
            QPointF(std::min(plotArea.right(), p1.x()), plotArea.bottom()));

        QColor fill("#0284c7");
        fill.setAlpha(30);
        painter->fillRect(selectionRect.normalized(), fill);
        painter->setPen(QPen(QColor("#0284c7"), 1.0, Qt::DashLine));
        painter->drawRect(selectionRect.normalized());
    }

    if (m_selectedClimbIndex >= 0 && m_selectedClimbIndex < static_cast<int>(m_climbs.size()))
    {
        const Climb& climb = m_climbs[static_cast<size_t>(m_selectedClimbIndex)];
        const QPointF p0 = chart()->mapToPosition(QPointF(climb.startSeconds / 60.0, m_axisY->min()), m_series);
        const QPointF p1 = chart()->mapToPosition(QPointF(climb.endSeconds / 60.0, m_axisY->min()), m_series);
        if (p1.x() >= plotArea.left() && p0.x() <= plotArea.right())
        {
            painter->setPen(QPen(QColor("#16a34a"), 1.5, Qt::SolidLine));
            painter->setBrush(Qt::NoBrush);
            painter->drawLine(QLineF(p0.x(), plotArea.top(), p0.x(), plotArea.bottom()));
            painter->drawLine(QLineF(p1.x(), plotArea.top(), p1.x(), plotArea.bottom()));

            if (m_climbEditingEnabled)
            {
                painter->setBrush(QColor("#15803d"));
                painter->setPen(QPen(Qt::white, 1.0));
                painter->drawEllipse(QPointF(p0.x(), plotArea.top() + climbHandleCenterOffsetYPx()), climbHandleRadiusPx(), climbHandleRadiusPx());
                painter->drawEllipse(QPointF(p1.x(), plotArea.top() + climbHandleCenterOffsetYPx()), climbHandleRadiusPx(), climbHandleRadiusPx());
            }
        }
    }
    painter->restore();
}