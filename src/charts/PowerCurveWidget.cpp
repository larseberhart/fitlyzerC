// SPDX-License-Identifier: GPL-3


#include "PowerCurveWidget.h"

#include <QtCharts/QCategoryAxis>
#include <QtCharts/QAbstractSeries>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QCursor>
#include <QPainter>
#include <QToolTip>

#include <algorithm>
#include <cmath>

static QString formatDurLabel(double seconds)
{
    const int s = static_cast<int>(seconds);
    if (s < 60)
        return QString("%1s").arg(s);
    if (s < 3600)
        return QString("%1m").arg(s / 60);
    const int h = s / 3600;
    const int m = (s % 3600) / 60;
    return m > 0
        ? QString("%1h%2m").arg(h).arg(m, 2, 10, QChar('0'))
        : QString("%1h").arg(h);
}

static std::vector<PowerCurvePoint> mergeWithDurations(const std::vector<double>& durations,
                                                       const std::vector<PowerCurvePoint>& points)
{
    std::vector<PowerCurvePoint> out;
    out.reserve(durations.size());
    for (double d : durations)
    {
        double best = 0.0;
        for (const PowerCurvePoint& p : points)
        {
            if (std::abs(p.durationSeconds - d) < 0.5)
            {
                best = p.bestPower;
                break;
            }
        }
        if (best > 0.0)
            out.push_back({d, best});
    }
    return out;
}

PowerCurveWidget::PowerCurveWidget(QWidget* parent)
    : QChartView(new QChart, parent)
{
    chart()->setTitle("Power Duration Curve");
    chart()->legend()->show();
    chart()->setAnimationOptions(QChart::NoAnimation);
    chart()->setMargins(QMargins(4, 4, 4, 4));
    setRenderHint(QPainter::Antialiasing);
    setMinimumHeight(240);
}

void PowerCurveWidget::clearChart()
{
    chart()->removeAllSeries();
    const auto axes = chart()->axes();
    for (auto* ax : axes) chart()->removeAxis(ax);
    chart()->setTitle("Power Duration Curve");
}

void PowerCurveWidget::setRideData(const RideData& rideData, double ftp)
{
    setRideDataWithComparisons(rideData, {}, {}, {}, ftp);
}

void PowerCurveWidget::setRideDataWithComparisons(const RideData& rideData,
                                                  const std::vector<PowerCurvePoint>& last90,
                                                  const std::vector<PowerCurvePoint>& currentYear,
                                                  const std::vector<PowerCurvePoint>& allTime,
                                                  double ftp)
{
    clearChart();

    const std::vector<double> durations = PowerCurve::standardDurations();
    auto pdcPts = mergeWithDurations(durations, PowerCurve::computeStandard(rideData));
    if (pdcPts.empty())
    {
        chart()->setTitle("Power Duration Curve (no power data)");
        return;
    }

    // Use integer indices on X so QCategoryAxis gives clean duration labels
    auto* lineSeries = new QLineSeries;
    lineSeries->setName("Best Power");

    auto* dotSeries = new QScatterSeries;
    dotSeries->setName("");
    dotSeries->setMarkerSize(9.0);
    dotSeries->setColor(QColor(255, 140, 0));

    double maxPower = 0.0;
    int idx = 0;
    for (const auto& pt : pdcPts)
    {
        lineSeries->append(idx, pt.bestPower);
        dotSeries->append(idx, pt.bestPower);
        maxPower = std::max(maxPower, pt.bestPower);
        ++idx;
    }

    chart()->addSeries(lineSeries);
    chart()->addSeries(dotSeries);

    auto addComparisonSeries = [this, &durations, &maxPower](const QString& name,
                                                              const QColor& color,
                                                              Qt::PenStyle style,
                                                              const std::vector<PowerCurvePoint>& pts)
    {
        if (pts.empty())
            return;

        const std::vector<PowerCurvePoint> merged = mergeWithDurations(durations, pts);
        if (merged.empty())
            return;

        auto* s = new QLineSeries;
        s->setName(name);
        QPen pen(color);
        pen.setWidthF(1.4);
        pen.setStyle(style);
        s->setPen(pen);

        int idx = 0;
        for (double d : durations)
        {
            for (const PowerCurvePoint& p : merged)
            {
                if (std::abs(p.durationSeconds - d) < 0.5)
                {
                    s->append(idx, p.bestPower);
                    maxPower = std::max(maxPower, p.bestPower);
                }
            }
            ++idx;
        }
        chart()->addSeries(s);
    };

    addComparisonSeries("Last 90 Days", QColor("#0ea5e9"), Qt::DashLine, last90);
    addComparisonSeries("Current Year", QColor("#14b8a6"), Qt::DotLine, currentYear);
    addComparisonSeries("All Time", QColor("#64748b"), Qt::DashDotLine, allTime);

    // X axis: category axis with formatted duration labels
    auto* axisX = new QCategoryAxis;
    axisX->setTitleText("Duration");
    axisX->setGridLineVisible(true);
    axisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    for (int i = 0; i < static_cast<int>(pdcPts.size()); ++i)
        axisX->append(formatDurLabel(pdcPts[i].durationSeconds), i);
    axisX->setRange(-0.5, idx - 0.5);
    chart()->addAxis(axisX, Qt::AlignBottom);
    for (QAbstractSeries* series : chart()->series())
        series->attachAxis(axisX);

    // Y axis
    auto* axisY = new QValueAxis;
    axisY->setTitleText("Power (W)");
    axisY->setLabelFormat("%.0f");
    axisY->setGridLineVisible(true);
    axisY->setRange(0, maxPower * 1.1);
    chart()->addAxis(axisY, Qt::AlignLeft);
    for (QAbstractSeries* series : chart()->series())
        series->attachAxis(axisY);

    // Optional FTP reference line
    if (ftp > 0.0 && ftp <= maxPower * 1.2)
    {
        auto* ftpLine = new QLineSeries;
        ftpLine->setName(QString("FTP (%1 W)").arg(ftp, 0, 'f', 0));
        ftpLine->setPen(QPen(Qt::red, 1.5, Qt::DashLine));
        ftpLine->append(-0.5, ftp);
        ftpLine->append(idx - 0.5, ftp);
        chart()->addSeries(ftpLine);
        ftpLine->attachAxis(axisX);
        ftpLine->attachAxis(axisY);
    }

    // Tooltip for scatter points
    connect(dotSeries, &QScatterSeries::hovered,
        this, [pdcPts](const QPointF& point, bool state)
        {
            if (state)
            {
                const int i =
                    static_cast<int>(std::round(point.x()));
                if (i >= 0 && i < static_cast<int>(pdcPts.size()))
                    QToolTip::showText(
                        QCursor::pos(),
                        formatDurLabel(pdcPts[i].durationSeconds) +
                        "\n" +
                        QString("%1 W").arg(
                            pdcPts[i].bestPower, 0, 'f', 0));
            }
            else
            {
                QToolTip::hideText();
            }
        });

    const double p3m = PowerCurve::bestMeanPower(rideData, 180.0);
    const double p12m = PowerCurve::bestMeanPower(rideData, 720.0);
    double cp = 0.0;
    double wPrime = 0.0;
    if (p3m > 0.0 && p12m > 0.0)
    {
        cp = ((p12m * 720.0) - (p3m * 180.0)) / (720.0 - 180.0);
        wPrime = std::max(0.0, (p3m - cp) * 180.0);
    }

    chart()->setTitle(
        QString("Power Duration Curve   CP %1 W   W' %2 kJ")
            .arg(cp, 0, 'f', 0)
            .arg(wPrime / 1000.0, 0, 'f', 1));
}