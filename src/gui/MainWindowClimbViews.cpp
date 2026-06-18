// SPDX-License-Identifier: GPL-3

#include "MainWindow.h"

#include <QBrush>
#include <QColor>
#include <QSignalBlocker>
#include <QTableWidgetItem>

#include <QtCharts/QAbstractAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>

#include "charts/RideChartWidget.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>

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

static constexpr int kClimbStartRole = Qt::UserRole + 3;
static constexpr int kClimbEndRole = Qt::UserRole + 4;

class ClimbNameTableItem final : public QTableWidgetItem
{
public:
    explicit ClimbNameTableItem(const QString& text)
        : QTableWidgetItem(text)
    {}

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

class ClimbSortKeyTableItem final : public QTableWidgetItem
{
public:
    explicit ClimbSortKeyTableItem(const QString& text, double sortKey)
        : QTableWidgetItem(text)
        , m_sortKey(sortKey)
    {}

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
} // namespace

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
