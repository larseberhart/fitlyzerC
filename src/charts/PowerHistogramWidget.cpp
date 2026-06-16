// SPDX-License-Identifier: GPL-3


#include "PowerHistogramWidget.h"

#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>
#include <QPainter>

#include <algorithm>
#include <cmath>
#include <map>

PowerHistogramWidget::PowerHistogramWidget(QWidget* parent)
    : QChartView(new QChart, parent)
{
    chart()->setTitle("Power Distribution");
    chart()->legend()->hide();
    chart()->setAnimationOptions(QChart::NoAnimation);
    chart()->setMargins(QMargins(4, 4, 4, 4));
    setRenderHint(QPainter::Antialiasing);
    setMinimumHeight(200);
}

void PowerHistogramWidget::clearChart()
{
    chart()->removeAllSeries();
    const auto axes = chart()->axes();
    for (auto* ax : axes) chart()->removeAxis(ax);
    chart()->setTitle("Power Distribution");
}

void PowerHistogramWidget::setRideData(const RideData& rideData)
{
    clearChart();

    constexpr double kBucketW = 25.0;   // watts per bucket

    std::map<int, double> buckets;      // bucket index → seconds

    const int n = static_cast<int>(rideData.records.size());
    for (int i = 0; i < n; ++i)
    {
        const auto& r = rideData.records[i];
        if (!r.hasPower || r.power <= 0.0) continue;

        double dt = 1.0;
        if (n > 1)
        {
            if (i == 0)
                dt = rideData.records[1].elapsedSeconds -
                     rideData.records[0].elapsedSeconds;
            else if (i == n - 1)
                dt = rideData.records[n - 1].elapsedSeconds -
                     rideData.records[n - 2].elapsedSeconds;
            else
                dt = (rideData.records[i + 1].elapsedSeconds -
                      rideData.records[i - 1].elapsedSeconds) / 2.0;
        }
        dt = std::max(0.0, dt);

        const int bucket = static_cast<int>(r.power / kBucketW);
        buckets[bucket] += dt;
    }

    if (buckets.empty())
    {
        chart()->setTitle("Power Distribution (no data)");
        return;
    }

    const int minBucket = buckets.begin()->first;
    const int maxBucket = buckets.rbegin()->first;
    const int numBuckets = maxBucket - minBucket + 1;

    auto* barSet = new QBarSet("Time (s)");
    QStringList categories;

    // Show axis labels every N buckets to avoid crowding
    const int labelStep = std::max(1, numBuckets / 10);

    for (int b = minBucket; b <= maxBucket; ++b)
    {
        const double sec = buckets.count(b) ? buckets.at(b) : 0.0;
        *barSet << sec;

        const int relIdx = b - minBucket;
        categories <<
            (relIdx % labelStep == 0
                ? QString::number(static_cast<int>(b * kBucketW))
                : QString(""));
    }

    auto* series = new QBarSeries;
    series->append(barSet);
    chart()->addSeries(series);

    auto* axisX = new QBarCategoryAxis;
    axisX->append(categories);
    axisX->setTitleText("Power (W)");
    axisX->setLabelsAngle(-45);
    chart()->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto* axisY = new QValueAxis;
    axisY->setTitleText("Time (s)");
    axisY->setLabelFormat("%.0f");
    axisY->setGridLineVisible(true);
    chart()->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart()->setTitle("Power Distribution");
}