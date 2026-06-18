// SPDX-License-Identifier: GPL-3

#include "MainWindow.h"

#include <QAction>
#include <QFont>
#include <QMenu>
#include <QMessageBox>
#include <QModelIndex>
#include <QStatusBar>
#include <QTableWidgetItem>

#include "analysis/ClimbDetector.h"
#include "analysis/ClimbMetrics.h"
#include "charts/RideChartWidget.h"
#include "core/undo/ClimbEditCommand.h"
#include "database/ClimbRepository.h"
#include "fit/RideData.h"
#include "maps/MapRenderer.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <memory>
#include <vector>

namespace
{
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
}

std::vector<int> MainWindow::selectedClimbIndicesFromTable() const
{
    std::vector<int> indices;
    if (!m_climbsTable || !m_climbsTable->selectionModel())
        return indices;

    const QModelIndexList rows = m_climbsTable->selectionModel()->selectedRows(0);
    for (const QModelIndex& modelIndex : rows)
    {
        auto* item = m_climbsTable->item(modelIndex.row(), 0);
        if (!item)
            continue;

        const double selectedStart = item->data(kClimbStartRole).toDouble();
        const double selectedEnd = item->data(kClimbEndRole).toDouble();

        for (int i = 0; i < static_cast<int>(m_detectedClimbs.size()); ++i)
        {
            const Climb& c = m_detectedClimbs[static_cast<size_t>(i)];
            if (std::abs(c.startSeconds - selectedStart) < 0.75 &&
                std::abs(c.endSeconds - selectedEnd) < 0.75)
            {
                indices.push_back(i);
                break;
            }
        }
    }

    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    return indices;
}

void MainWindow::onClimbTableContextMenuRequested(const QPoint& pos)
{
    if (!m_climbsTable)
        return;

    const int clickedRow = m_climbsTable->rowAt(pos.y());
    if (clickedRow >= 0 && !m_climbsTable->item(clickedRow, 0)->isSelected())
    {
        m_climbsTable->clearSelection();
        m_climbsTable->setCurrentCell(clickedRow, 0);
        m_climbsTable->selectRow(clickedRow);
    }

    const std::vector<int> selected = selectedClimbIndicesFromTable();
    if (selected.empty())
        return;

    QMenu menu(m_climbsTable);
    QAction* editAct = menu.addAction("Edit");
    QAction* removeAct = menu.addAction("Remove");
    QAction* joinAct = menu.addAction("Join");

    editAct->setEnabled(selected.size() == 1);
    joinAct->setEnabled(selected.size() >= 2);
    removeAct->setEnabled(!selected.empty());

    QAction* chosen = menu.exec(m_climbsTable->viewport()->mapToGlobal(pos));
    if (!chosen)
        return;

    if (chosen == editAct)
        editSelectedClimbBoundaries();
    else if (chosen == removeAct)
        removeSelectedClimbs();
    else if (chosen == joinAct)
        joinSelectedClimbs();
}

void MainWindow::removeSelectedClimbs()
{
    std::vector<int> selected = selectedClimbIndicesFromTable();
    if (selected.empty())
        return;

    if (m_dbManager.isOpen() && m_controller->currentActivityId() > 0)
    {
        auto db = m_dbManager.database();
        ClimbRepository repo(db);
        for (int idx : selected)
        {
            if (idx >= 0 && idx < static_cast<int>(m_detectedClimbs.size()))
            {
                const int climbId = m_detectedClimbs[static_cast<size_t>(idx)].id;
                if (climbId > 0)
                    repo.softDeleteClimb(climbId);
            }
        }
    }

    std::sort(selected.rbegin(), selected.rend());
    for (int idx : selected)
    {
        if (idx >= 0 && idx < static_cast<int>(m_detectedClimbs.size()))
            m_detectedClimbs.erase(m_detectedClimbs.begin() + idx);
    }

    for (int i = 0; i < static_cast<int>(m_detectedClimbs.size()); ++i)
        m_detectedClimbs[static_cast<size_t>(i)].name = QString("Climb %1").arg(i + 1);

    refreshClimbViews();
}

void MainWindow::editSelectedClimbBoundaries()
{
    std::vector<int> selected = selectedClimbIndicesFromTable();
    if (selected.size() != 1)
        return;

    const int idx = selected.front();
    if (idx < 0 || idx >= static_cast<int>(m_detectedClimbs.size()))
        return;

    const Climb& climb = m_detectedClimbs[static_cast<size_t>(idx)];

    if (m_climbAltitudeChart)
    {
        m_climbAltitudeChart->setClimbEditingEnabled(true);
        m_climbAltitudeChart->setSelectedClimbIndex(idx);

        const double padding = std::max(5.0, climb.durationSeconds * 0.08);
        const double rangeStart = std::max(0.0, climb.startSeconds - padding);
        const double rangeEnd = climb.endSeconds + padding;
        m_climbAltitudeChart->setXRange(rangeStart / 60.0, rangeEnd / 60.0);
        m_climbAltitudeChart->setCrosshair(climb.startSeconds / 60.0);
        m_climbAltitudeChart->setFocus();
    }

    statusBar()->showMessage(
        "Graphical edit enabled: drag the climb start/end handles in the elevation chart.",
        5000);
}

void MainWindow::joinSelectedClimbs()
{
    std::vector<int> selected = selectedClimbIndicesFromTable();
    if (selected.size() < 2)
        return;

    std::sort(selected.begin(), selected.end());
    const int keepIdx = selected.front();
    if (keepIdx < 0 || keepIdx >= static_cast<int>(m_detectedClimbs.size()))
        return;

    double newStart = m_detectedClimbs[static_cast<size_t>(keepIdx)].startSeconds;
    double newEnd = m_detectedClimbs[static_cast<size_t>(keepIdx)].endSeconds;
    for (int idx : selected)
    {
        if (idx < 0 || idx >= static_cast<int>(m_detectedClimbs.size()))
            continue;
        const Climb& c = m_detectedClimbs[static_cast<size_t>(idx)];
        newStart = std::min(newStart, c.startSeconds);
        newEnd = std::max(newEnd, c.endSeconds);
    }

    const Climb keep = m_detectedClimbs[static_cast<size_t>(keepIdx)];
    onClimbBoundaryEdited(keep.startSeconds, keep.endSeconds, newStart, newEnd);

    if (m_dbManager.isOpen() && m_controller->currentActivityId() > 0)
    {
        auto db = m_dbManager.database();
        ClimbRepository repo(db);
        std::sort(selected.rbegin(), selected.rend());
        for (int idx : selected)
        {
            if (idx == keepIdx)
                continue;
            if (idx >= 0 && idx < static_cast<int>(m_detectedClimbs.size()))
            {
                const int climbId = m_detectedClimbs[static_cast<size_t>(idx)].id;
                if (climbId > 0)
                    repo.softDeleteClimb(climbId);
                m_detectedClimbs.erase(m_detectedClimbs.begin() + idx);
            }
        }
    }
    else
    {
        std::sort(selected.rbegin(), selected.rend());
        for (int idx : selected)
        {
            if (idx == keepIdx)
                continue;
            if (idx >= 0 && idx < static_cast<int>(m_detectedClimbs.size()))
                m_detectedClimbs.erase(m_detectedClimbs.begin() + idx);
        }
    }

    for (int i = 0; i < static_cast<int>(m_detectedClimbs.size()); ++i)
        m_detectedClimbs[static_cast<size_t>(i)].name = QString("Climb %1").arg(i + 1);

    refreshClimbViews(newStart, newEnd);
}

void MainWindow::updateClimbRowStyles()
{
    if (!m_climbsTable)
        return;

    const int selectedRow = m_climbsTable->currentRow();
    for (int row = 0; row < m_climbsTable->rowCount(); ++row)
    {
        for (int col = 0; col < m_climbsTable->columnCount(); ++col)
        {
            auto* item = m_climbsTable->item(row, col);
            if (!item)
                continue;

            QFont font = item->font();
            font.setBold(row == selectedRow);
            item->setFont(font);
        }
    }
}

void MainWindow::onClimbSelectionChanged()
{
    if (!m_climbsTable)
        return;

    const int row = m_climbsTable->currentRow();
    if (row < 0)
    {
        if (m_climbAltitudeChart)
            m_climbAltitudeChart->setSelectedClimbIndex(-1);
        updateClimbQuarterCharts(nullptr);
        updateClimbRowStyles();
        if (m_climbSummaryLabel)
            m_climbSummaryLabel->setText("Climb summary: select a climb");
        return;
    }

    auto* firstItem = m_climbsTable->item(row, 0);
    if (!firstItem)
        return;

    const double selectedStart = firstItem->data(kClimbStartRole).toDouble();
    const double selectedEnd = firstItem->data(kClimbEndRole).toDouble();

    int climbIndex = -1;
    for (int i = 0; i < static_cast<int>(m_detectedClimbs.size()); ++i)
    {
        const Climb& c = m_detectedClimbs[static_cast<size_t>(i)];
        if (std::abs(c.startSeconds - selectedStart) < 0.75 &&
            std::abs(c.endSeconds - selectedEnd) < 0.75)
        {
            climbIndex = i;
            break;
        }
    }

    if (climbIndex < 0)
    {
        if (m_climbAltitudeChart)
            m_climbAltitudeChart->setSelectedClimbIndex(-1);
        updateClimbQuarterCharts(nullptr);
        return;
    }

    if (m_climbAltitudeChart)
        m_climbAltitudeChart->setSelectedClimbIndex(climbIndex);

    const Climb& climb = m_detectedClimbs[static_cast<size_t>(climbIndex)];
    updateClimbQuarterCharts(&climb);
    const double padding = std::max(5.0, climb.durationSeconds * 0.08);
    const double startSeconds = std::max(0.0, climb.startSeconds - padding);
    const double endSeconds = climb.endSeconds + padding;

    (void)startSeconds;
    (void)endSeconds;

    if (m_climbSummaryLabel)
    {
        const QString wkgText = climb.averageWattsPerKg > 0.0
            ? QString::number(climb.averageWattsPerKg, 'f', 2)
            : QStringLiteral("\xE2\x80\x94");
        m_climbSummaryLabel->setText(
            QString("%1 | %2 | Gain %3 m | Avg %4%% | W/kg %5 (%6) | VAM %7 m/h | Fade %8%% | Decouple %9%% | %10 | %11")
                .arg(climb.name)
                .arg(fmtDur(climb.durationSeconds))
                .arg(climb.elevationGainM, 0, 'f', 0)
                .arg(climb.averageGradient, 0, 'f', 1)
                .arg(wkgText)
                .arg(climb.wattsPerKgRank.isEmpty() ? QStringLiteral("\xE2\x80\x94") : climb.wattsPerKgRank)
                .arg(climb.vam, 0, 'f', 0)
                .arg(climb.powerFadePct, 0, 'f', 1)
                .arg(climb.aerobicDecouplingPct, 0, 'f', 1)
                .arg(climb.category)
                .arg(climb.shapeClass));
    }

    updateClimbRowStyles();
}

void MainWindow::onClimbRowDoubleClicked(QTableWidgetItem* item)
{
    if (!item || !m_climbsTable)
        return;

    const int row = item->row();
    auto* firstItem = m_climbsTable->item(row, 0);
    if (!firstItem)
        return;

    const double selectedStart = firstItem->data(kClimbStartRole).toDouble();
    const double selectedEnd = firstItem->data(kClimbEndRole).toDouble();

    int climbIndex = -1;
    for (int i = 0; i < static_cast<int>(m_detectedClimbs.size()); ++i)
    {
        const Climb& c = m_detectedClimbs[static_cast<size_t>(i)];
        if (std::abs(c.startSeconds - selectedStart) < 0.75 &&
            std::abs(c.endSeconds - selectedEnd) < 0.75)
        {
            climbIndex = i;
            break;
        }
    }

    if (climbIndex < 0)
        return;

    const Climb& climb = m_detectedClimbs[static_cast<size_t>(climbIndex)];
    const double padding = std::max(5.0, climb.durationSeconds * 0.08);
    const double rangeStart = std::max(0.0, climb.startSeconds - padding);
    const double rangeEnd = climb.endSeconds + padding;

    const std::initializer_list<RideChartWidget*> all =
        { m_powerChart, m_hrChart, m_cadenceChart, m_speedChart, m_altitudeChart,
          m_climbPowerChart, m_climbHrChart, m_climbCadenceChart, m_climbSpeedChart, m_climbAltitudeChart };
    for (auto* chart : all)
    {
        if (!chart)
            continue;
        chart->setXRange(rangeStart / 60.0, rangeEnd / 60.0);
        chart->setCrosshair(climb.startSeconds / 60.0);
    }

    if (m_mapRenderer)
    {
        m_mapRenderer->setVisibleTimeRange(climb.startSeconds, climb.endSeconds);
        m_mapRenderer->fitToVisibleRange();
        m_mapRenderer->setCurrentTime(climb.startSeconds);
    }
}

void MainWindow::onClimbBoundaryEdited(
    double oldStartSeconds,
    double oldEndSeconds,
    double newStartSeconds,
    double newEndSeconds)
{
    if (m_detectedClimbs.empty())
        return;

    int idx = -1;
    for (int i = 0; i < static_cast<int>(m_detectedClimbs.size()); ++i)
    {
        const Climb& c = m_detectedClimbs[static_cast<size_t>(i)];
        if (std::abs(c.startSeconds - oldStartSeconds) < 0.75 &&
            std::abs(c.endSeconds - oldEndSeconds) < 0.75)
        {
            idx = i;
            break;
        }
    }

    if (idx < 0)
        idx = m_climbsTable ? m_climbsTable->currentRow() : -1;
    if (idx < 0 || idx >= static_cast<int>(m_detectedClimbs.size()))
        return;

    const auto& ride = m_controller->rideData();
    if (ride.records.empty())
        return;

    auto nearestIndexForSecond = [&ride](double sec)
    {
        auto it = std::lower_bound(
            ride.records.begin(),
            ride.records.end(),
            sec,
            [](const RideRecord& r, double s) { return r.elapsedSeconds < s; });

        if (it == ride.records.end())
            return static_cast<int>(ride.records.size()) - 1;

        int index = static_cast<int>(std::distance(ride.records.begin(), it));
        if (index > 0)
        {
            const double currDist = std::abs(ride.records[static_cast<size_t>(index)].elapsedSeconds - sec);
            const double prevDist = std::abs(ride.records[static_cast<size_t>(index - 1)].elapsedSeconds - sec);
            if (prevDist < currDist)
                --index;
        }
        return index;
    };

    int startIdx = nearestIndexForSecond(newStartSeconds);
    int endIdx = nearestIndexForSecond(newEndSeconds);
    if (endIdx < startIdx)
        std::swap(startIdx, endIdx);
    if (endIdx <= startIdx)
        endIdx = std::min(static_cast<int>(ride.records.size()) - 1, startIdx + 1);

    const std::vector<double> cumulative = ClimbDetector::buildCumulativeDistanceMeters(ride);
    const std::vector<double> smoothed = ClimbDetector::smoothAltitudeByDistance(
        ride,
        cumulative,
        m_climbSmoothingSpin ? m_climbSmoothingSpin->value() : 50.0);
    const std::vector<double> gradient = ClimbDetector::computeLocalGradientPct(smoothed, cumulative, 25.0);

    Climb rebuilt = ClimbMetrics::build(
        ride,
        cumulative,
        smoothed,
        gradient,
        startIdx,
        endIdx,
        idx + 1);

    assignClimbWeightMetrics(rebuilt, resolveActiveActivityWeightKg());

    rebuilt.name = m_detectedClimbs[static_cast<size_t>(idx)].name;
    rebuilt.id   = m_detectedClimbs[static_cast<size_t>(idx)].id;

    if (m_undoManager && m_dbManager.isOpen() && rebuilt.id > 0)
    {
        auto db = m_dbManager.database();
        ClimbRepository repo(db);
        const ClimbRecord beforeRecord = repo.getClimb(rebuilt.id);
        auto refresh = [this]() {
            m_controller->reloadClimbsFromDatabase();
            m_detectedClimbs = m_controller->climbs();
            applyWeightMetricsToClimbs(m_detectedClimbs, resolveActiveActivityWeightKg());
            refreshClimbViews();
        };
        ClimbRecord afterRecord = beforeRecord;
        afterRecord.startSeconds    = rebuilt.startSeconds;
        afterRecord.endSeconds      = rebuilt.endSeconds;
        afterRecord.elevationGainM  = rebuilt.elevationGainM;
        afterRecord.lengthKm        = rebuilt.lengthKm;
        afterRecord.averageGradient = rebuilt.averageGradient;
        afterRecord.avgPower        = rebuilt.averagePower;
        afterRecord.np              = rebuilt.normalizedPower;
        afterRecord.avgHr           = rebuilt.averageHeartRate;
        afterRecord.avgCadence      = rebuilt.averageCadence;
        afterRecord.vam             = rebuilt.vam;
        afterRecord.source          = QStringLiteral("edited");
        afterRecord.locked          = true;
        m_undoManager->push(std::make_unique<ClimbEditCommand>(
            beforeRecord, afterRecord,
            m_dbManager.database().connectionName(),
            refresh));
    }

    m_detectedClimbs[static_cast<size_t>(idx)] = rebuilt;

    if (m_dbManager.isOpen() && m_controller->currentActivityId() > 0)
    {
        auto db = m_dbManager.database();
        ClimbRepository repo(db);
        const Climb& c = m_detectedClimbs[static_cast<size_t>(idx)];
        if (c.id > 0)
        {
            ClimbRecord record;
            record.id              = c.id;
            record.activityId      = m_controller->currentActivityId();
            record.startSeconds    = c.startSeconds;
            record.endSeconds      = c.endSeconds;
            record.name            = c.name;
            record.elevationGainM  = c.elevationGainM;
            record.lengthKm        = c.lengthKm;
            record.averageGradient = c.averageGradient;
            record.source          = QStringLiteral("edited");
            record.locked          = true;
            repo.updateClimb(record);
        }
        else
        {
            ClimbRecord record;
            record.activityId      = m_controller->currentActivityId();
            record.startSeconds    = c.startSeconds;
            record.endSeconds      = c.endSeconds;
            record.name            = c.name;
            record.elevationGainM  = c.elevationGainM;
            record.lengthKm        = c.lengthKm;
            record.averageGradient = c.averageGradient;
            record.source          = QStringLiteral("edited");
            record.locked          = true;
            m_detectedClimbs[static_cast<size_t>(idx)].id = repo.insertClimb(record);
        }
    }

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

    if (m_climbsTable)
    {
        auto setCell = [this, idx](int col, const QString& text)
        {
            auto* item = m_climbsTable->item(idx, col);
            if (!item)
            {
                item = new QTableWidgetItem;
                m_climbsTable->setItem(idx, col, item);
            }
            item->setText(text);
            item->setTextAlignment(Qt::AlignCenter);
        };

        auto setNumericCell = [this, idx](int col, double value, int decimals, const QString& fallback = QStringLiteral("\xE2\x80\x94"), bool alwaysNumeric = false)
        {
            QTableWidgetItem* item = nullptr;
            if (alwaysNumeric || value > 0.0)
                item = new ClimbSortKeyTableItem(QString::number(value, 'f', decimals), value);
            else
                item = new ClimbSortKeyTableItem(fallback, std::numeric_limits<double>::quiet_NaN());
            item->setTextAlignment(Qt::AlignCenter);
            m_climbsTable->setItem(idx, col, item);
        };

        auto* nameItem = m_climbsTable->item(idx, 0);
        if (!nameItem)
        {
            nameItem = new ClimbNameTableItem(rebuilt.name);
            m_climbsTable->setItem(idx, 0, nameItem);
        }
        nameItem->setData(kClimbStartRole, rebuilt.startSeconds);
        nameItem->setData(kClimbEndRole, rebuilt.endSeconds);

        setNumericCell(1, rebuilt.lengthKm, 2, QStringLiteral("0.00"), true);
        setNumericCell(2, rebuilt.elevationGainM, 0, QStringLiteral("0"), true);
        setNumericCell(3, rebuilt.averageGradient, 1, QStringLiteral("0.0"), true);
        setNumericCell(4, rebuilt.maximumGradient, 1, QStringLiteral("0.0"), true);
        m_climbsTable->setItem(idx, 5, new ClimbSortKeyTableItem(fmtDur(rebuilt.durationSeconds), rebuilt.durationSeconds));
        m_climbsTable->item(idx, 5)->setTextAlignment(Qt::AlignCenter);
        setNumericCell(6, rebuilt.averagePower, 0);
        setNumericCell(7, rebuilt.averageWattsPerKg, 2);
        setCell(8, rebuilt.wattsPerKgRank.isEmpty() ? QStringLiteral("\xE2\x80\x94") : rebuilt.wattsPerKgRank);
        setNumericCell(9, rebuilt.normalizedPower, 0);
        setNumericCell(10, rebuilt.averageHeartRate, 0);
        setNumericCell(11, rebuilt.averageCadence, 0);
        setNumericCell(12, rebuilt.averageSpeed, 1);
        setNumericCell(13, rebuilt.vam, 0);
        setNumericCell(14, rebuilt.powerFadePct, 1, QStringLiteral("0.0"), true);
        setNumericCell(15, rebuilt.aerobicDecouplingPct, 1, QStringLiteral("0.0"), true);
        setNumericCell(16, rebuilt.difficultyScore, 0, QStringLiteral("0"), true);
        setCell(17, rebuilt.category.isEmpty() ? QStringLiteral("\xE2\x80\x94") : rebuilt.category);
        setCell(18, rebuilt.shapeClass.isEmpty() ? QStringLiteral("\xE2\x80\x94") : rebuilt.shapeClass);
    }

    onClimbSelectionChanged();
}

void MainWindow::revertSelectedClimbToAuto()
{
    std::vector<int> selected = selectedClimbIndicesFromTable();
    if (selected.size() != 1)
        return;

    const int idx = selected.front();
    if (idx < 0 || idx >= static_cast<int>(m_detectedClimbs.size()))
        return;

    if (!m_dbManager.isOpen() || m_controller->currentActivityId() <= 0)
        return;

    const int climbId = m_detectedClimbs[static_cast<size_t>(idx)].id;
    if (climbId <= 0)
        return;

    auto db = m_dbManager.database();
    ClimbRepository repo(db);
    ClimbRecord record = repo.getClimb(climbId);
    if (record.id <= 0)
        return;

    if (record.originalStartSeconds >= 0.0)
        record.startSeconds = record.originalStartSeconds;
    if (record.originalEndSeconds >= 0.0)
        record.endSeconds = record.originalEndSeconds;
    record.source = QStringLiteral("auto");
    record.locked = false;
    record.deleted = false;

    if (!repo.updateClimb(record))
        return;

    m_controller->reloadClimbsFromDatabase();
    m_detectedClimbs = m_controller->climbs();
    applyWeightMetricsToClimbs(m_detectedClimbs, resolveActiveActivityWeightKg());
    refreshClimbViews(record.startSeconds, record.endSeconds);
}

void MainWindow::onNewClimbRequested(double startSeconds, double endSeconds)
{
    if (!m_controller)
        return;

    const auto& ride = m_controller->rideData();
    if (ride.records.empty())
        return;

    const double orderedStart = std::min(startSeconds, endSeconds);
    const double orderedEnd = std::max(startSeconds, endSeconds);
    if (orderedEnd - orderedStart < 1.0)
        return;

    auto nearestIndexForSecond = [&ride](double sec)
    {
        auto it = std::lower_bound(
            ride.records.begin(),
            ride.records.end(),
            sec,
            [](const RideRecord& r, double s) { return r.elapsedSeconds < s; });

        if (it == ride.records.end())
            return static_cast<int>(ride.records.size()) - 1;

        int index = static_cast<int>(std::distance(ride.records.begin(), it));
        if (index > 0)
        {
            const double currDist = std::abs(ride.records[static_cast<size_t>(index)].elapsedSeconds - sec);
            const double prevDist = std::abs(ride.records[static_cast<size_t>(index - 1)].elapsedSeconds - sec);
            if (prevDist < currDist)
                --index;
        }
        return index;
    };

    int startIdx = nearestIndexForSecond(orderedStart);
    int endIdx = nearestIndexForSecond(orderedEnd);
    if (endIdx < startIdx)
        std::swap(startIdx, endIdx);
    if (endIdx <= startIdx)
        endIdx = std::min(static_cast<int>(ride.records.size()) - 1, startIdx + 1);

    const std::vector<double> cumulative = ClimbDetector::buildCumulativeDistanceMeters(ride);
    const std::vector<double> smoothed = ClimbDetector::smoothAltitudeByDistance(
        ride,
        cumulative,
        m_climbSmoothingSpin ? m_climbSmoothingSpin->value() : 50.0);
    const std::vector<double> gradient = ClimbDetector::computeLocalGradientPct(smoothed, cumulative, 25.0);

    Climb newClimb = ClimbMetrics::build(
        ride,
        cumulative,
        smoothed,
        gradient,
        startIdx,
        endIdx,
        static_cast<int>(m_detectedClimbs.size()) + 1);

    assignClimbWeightMetrics(newClimb, resolveActiveActivityWeightKg());

    if (m_dbManager.isOpen() && m_controller->currentActivityId() > 0)
    {
        auto db = m_dbManager.database();
        ClimbRepository repo(db);
        ClimbRecord record;
        record.activityId      = m_controller->currentActivityId();
        record.startSeconds    = newClimb.startSeconds;
        record.endSeconds      = newClimb.endSeconds;
        record.name            = newClimb.name;
        record.elevationGainM  = newClimb.elevationGainM;
        record.lengthKm        = newClimb.lengthKm;
        record.averageGradient = newClimb.averageGradient;
        record.source          = QStringLiteral("manual");
        record.locked          = true;
        newClimb.id = repo.insertClimb(record);
    }

    m_detectedClimbs.push_back(newClimb);
    std::sort(
        m_detectedClimbs.begin(),
        m_detectedClimbs.end(),
        [](const Climb& a, const Climb& b)
        {
            if (std::abs(a.startSeconds - b.startSeconds) > 0.1)
                return a.startSeconds < b.startSeconds;
            return a.endSeconds < b.endSeconds;
        });

    for (int i = 0; i < static_cast<int>(m_detectedClimbs.size()); ++i)
        m_detectedClimbs[static_cast<size_t>(i)].name = QString("Climb %1").arg(i + 1);

    refreshClimbViews(newClimb.startSeconds, newClimb.endSeconds);
}
