// SPDX-License-Identifier: GPL-3

/**
 * @file FitnessChartWidget.cpp
 * @brief Chart widget and visualization support for FitnessChartWidget.
 *
 * Provides chart rendering or chart-related data types used to visualize ride and fitness metrics in the UI.
 *
 * Responsibilities:
 * - Provide chart visualization behavior or chart support types
 *
 * @author Lars EBERHART
 */

#include "FitnessChartWidget.h"

#include <QPen>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <algorithm>

FitnessChartWidget::FitnessChartWidget(QWidget* parent)
    : QChartView(new QChart, parent)
{
    m_ctlSeries = new QLineSeries;
    m_atlSeries = new QLineSeries;
    m_tsbSeries = new QLineSeries;

    m_ctlSeries->setName("Fitness (CTL)");
    m_atlSeries->setName("Fatigue (ATL)");
    m_tsbSeries->setName("Form (TSB)");

    m_ctlSeries->setPen(QPen(QColor("#16a34a"), 2.0));
    m_atlSeries->setPen(QPen(QColor("#dc2626"), 2.0));
    m_tsbSeries->setPen(QPen(QColor("#2563eb"), 1.8, Qt::DashLine));

    m_axisX = new QValueAxis;
    m_axisY = new QValueAxis;
    m_axisX->setLabelFormat("%.0f");
    m_axisX->setTitleText("Day Index");
    m_axisY->setLabelFormat("%.0f");
    m_axisY->setTitleText("Load");

    chart()->addSeries(m_ctlSeries);
    chart()->addSeries(m_atlSeries);
    chart()->addSeries(m_tsbSeries);
    chart()->addAxis(m_axisX, Qt::AlignBottom);
    chart()->addAxis(m_axisY, Qt::AlignLeft);

    m_ctlSeries->attachAxis(m_axisX);
    m_ctlSeries->attachAxis(m_axisY);
    m_atlSeries->attachAxis(m_axisX);
    m_atlSeries->attachAxis(m_axisY);
    m_tsbSeries->attachAxis(m_axisX);
    m_tsbSeries->attachAxis(m_axisY);

    chart()->setTitle("Fitness / Fatigue / Form");
    chart()->legend()->setVisible(true);
    chart()->setAnimationOptions(QChart::NoAnimation);

    setRenderHint(QPainter::Antialiasing);
    setMinimumHeight(280);
}

void FitnessChartWidget::setTimeline(const std::vector<FitnessMetricsPoint>& timeline)
{
    m_ctlSeries->clear();
    m_atlSeries->clear();
    m_tsbSeries->clear();

    if (timeline.empty())
    {
        m_axisX->setRange(0.0, 1.0);
        m_axisY->setRange(-10.0, 10.0);
        return;
    }

    double minY = 1e9;
    double maxY = -1e9;

    for (size_t i = 0; i < timeline.size(); ++i)
    {
        const double x = static_cast<double>(i);
        const auto& p = timeline[i];

        m_ctlSeries->append(x, p.ctl);
        m_atlSeries->append(x, p.atl);
        m_tsbSeries->append(x, p.tsb);

        minY = std::min(minY, std::min(p.tsb, std::min(p.ctl, p.atl)));
        maxY = std::max(maxY, std::max(p.tsb, std::max(p.ctl, p.atl)));
    }

    if (maxY <= minY)
        maxY = minY + 1.0;

    const double pad = std::max(5.0, (maxY - minY) * 0.1);
    m_axisX->setRange(0.0, static_cast<double>(timeline.size() - 1));
    m_axisY->setRange(minY - pad, maxY + pad);
}

void FitnessChartWidget::clearChart()
{
    m_ctlSeries->clear();
    m_atlSeries->clear();
    m_tsbSeries->clear();
    m_axisX->setRange(0.0, 1.0);
    m_axisY->setRange(-10.0, 10.0);
}