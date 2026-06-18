// SPDX-License-Identifier: GPL-3

#include "MainWindow.h"

#include <QBrush>
#include <QColor>
#include <QDateTime>
#include <QFont>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTableWidgetItem>

#include "charts/RideChartWidget.h"
#include "core/settings/DateFormatter.h"
#include "database/ActivityRepository.h"
#include "database/IntervalRepository.h"
#include "fit/RideData.h"
#include "maps/MapRenderer.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
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

static constexpr int kIntervalStartRole = Qt::UserRole + 1;
static constexpr int kIntervalEndRole = Qt::UserRole + 2;
}

void MainWindow::onIntervalSelectionChanged()
{
    if (!m_intervalsTable)
        return;

    const int row = m_intervalsTable->currentRow();
    if (row < 0)
    {
        updateIntervalRowStyles();
        clearIntervalSummary();
        return;
    }

    auto* firstItem = m_intervalsTable->item(row, 0);
    if (!firstItem)
    {
        updateIntervalRowStyles();
        clearIntervalSummary();
        return;
    }

    const QVariant startVar = firstItem->data(kIntervalStartRole);
    const QVariant endVar = firstItem->data(kIntervalEndRole);
    if (!startVar.isValid() || !endVar.isValid())
    {
        updateIntervalRowStyles();
        clearIntervalSummary();
        return;
    }

    const double startSeconds = startVar.toDouble();
    const double endSeconds = endVar.toDouble();

    updateIntervalRowStyles();
    updateIntervalSummary(startSeconds, endSeconds);

    if (m_followIntervalSelectionCheck && m_followIntervalSelectionCheck->isChecked())
        navigateToInterval(startSeconds, endSeconds, false);
}

void MainWindow::onIntervalRowDoubleClicked(QTableWidgetItem* item)
{
    if (!item || !m_intervalsTable)
        return;

    const int row = item->row();
    auto* firstItem = m_intervalsTable->item(row, 0);
    if (!firstItem)
        return;

    const QVariant startVar = firstItem->data(kIntervalStartRole);
    const QVariant endVar = firstItem->data(kIntervalEndRole);
    if (!startVar.isValid() || !endVar.isValid())
        return;

    if (m_followIntervalSelectionCheck && m_followIntervalSelectionCheck->isChecked())
        navigateToInterval(startVar.toDouble(), endVar.toDouble(), true);
}

void MainWindow::navigateToInterval(double startSeconds, double endSeconds, bool exactZoom)
{
    if (endSeconds <= startSeconds)
        return;

    const double duration = endSeconds - startSeconds;
    const double padding = exactZoom ? 0.0 : std::max(duration * 0.10, 5.0);
    const double rangeStart = std::max(0.0, startSeconds - padding);
    const double rangeEnd = endSeconds + padding;

    const std::initializer_list<RideChartWidget*> all =
        { m_powerChart, m_hrChart, m_cadenceChart, m_speedChart, m_altitudeChart };
    for (auto* chart : all)
    {
        if (!chart)
            continue;
        chart->setXRange(rangeStart / 60.0, rangeEnd / 60.0);
        chart->setCrosshair(startSeconds / 60.0);
    }

    if (m_mapRenderer)
    {
        m_mapRenderer->setVisibleTimeRange(startSeconds, endSeconds);
        m_mapRenderer->fitToVisibleRange();
        m_mapRenderer->setCurrentTime(startSeconds);
    }

    const QString modeLabel = exactZoom ? "exact" : "padded";
    statusBar()->showMessage(
        QString("Interval inspection: %1 zoom (%2-%3)")
            .arg(modeLabel)
            .arg(fmtDur(rangeStart))
            .arg(fmtDur(rangeEnd)),
        2500);
}

void MainWindow::selectIntervalRowOffset(int delta)
{
    if (!m_intervalsTable)
        return;

    const int rows = m_intervalsTable->rowCount();
    if (rows <= 0)
        return;

    int row = m_intervalsTable->currentRow();
    if (row < 0)
        row = 0;
    else
        row = std::clamp(row + delta, 0, rows - 1);

    m_intervalsTable->setCurrentCell(row, 0);
}

void MainWindow::updateIntervalRowStyles()
{
    if (!m_intervalsTable)
        return;

    const int selectedRow = m_intervalsTable->currentRow();
    for (int row = 0; row < m_intervalsTable->rowCount(); ++row)
    {
        for (int col = 0; col < m_intervalsTable->columnCount(); ++col)
        {
            auto* item = m_intervalsTable->item(row, col);
            if (!item)
                continue;

            QFont font = item->font();
            font.setBold(row == selectedRow);
            item->setFont(font);
        }
    }
}

void MainWindow::clearIntervalSummary()
{
    if (m_intervalSummaryLabel)
        m_intervalSummaryLabel->setText("Interval summary: select an interval");
}

void MainWindow::updateIntervalSummary(double startSeconds, double endSeconds)
{
    if (!m_intervalSummaryLabel || endSeconds <= startSeconds)
    {
        clearIntervalSummary();
        return;
    }

    const auto& records = m_controller->rideData().records;
    double sumPower = 0.0;
    int countPower = 0;
    double sumHr = 0.0;
    int countHr = 0;
    double sumCadence = 0.0;
    int countCadence = 0;
    double sumSpeed = 0.0;
    int countSpeed = 0;
    double elevationGain = 0.0;
    std::vector<double> powerValues;

    bool havePreviousAltitude = false;
    double previousAltitude = 0.0;

    for (const RideRecord& record : records)
    {
        if (record.elapsedSeconds < startSeconds || record.elapsedSeconds > endSeconds)
            continue;

        if (record.hasPower)
        {
            sumPower += record.power;
            ++countPower;
            powerValues.push_back(record.power);
        }
        if (record.hasHeartRate)
        {
            sumHr += record.heartRate;
            ++countHr;
        }
        if (record.hasCadence)
        {
            sumCadence += record.cadence;
            ++countCadence;
        }
        if (record.hasSpeed)
        {
            sumSpeed += record.speed;
            ++countSpeed;
        }
        if (record.hasAltitude)
        {
            if (havePreviousAltitude)
            {
                const double delta = record.altitude - previousAltitude;
                if (delta > 0.0)
                    elevationGain += delta;
            }
            previousAltitude = record.altitude;
            havePreviousAltitude = true;
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

    const double avgPower = countPower > 0 ? (sumPower / static_cast<double>(countPower)) : 0.0;
    const double avgHr = countHr > 0 ? (sumHr / static_cast<double>(countHr)) : 0.0;
    const double avgCadence = countCadence > 0 ? (sumCadence / static_cast<double>(countCadence)) : 0.0;
    const double avgSpeed = countSpeed > 0 ? (sumSpeed / static_cast<double>(countSpeed)) : 0.0;

    m_intervalSummaryLabel->setText(
        QString("Interval %1 | Avg P %2 | NP %3 | HR %4 | Cad %5 | Speed %6 | Elev +%7")
            .arg(fmtDur(std::max(0.0, endSeconds - startSeconds)))
            .arg(avgPower > 0.0 ? QString("%1 W").arg(avgPower, 0, 'f', 0) : QString("\xE2\x80\x94"))
            .arg(np > 0.0 ? QString("%1 W").arg(np, 0, 'f', 0) : QString("\xE2\x80\x94"))
            .arg(avgHr > 0.0 ? QString("%1 bpm").arg(avgHr, 0, 'f', 0) : QString("\xE2\x80\x94"))
            .arg(avgCadence > 0.0 ? QString("%1 rpm").arg(avgCadence, 0, 'f', 0) : QString("\xE2\x80\x94"))
            .arg(avgSpeed > 0.0 ? QString("%1 km/h").arg(avgSpeed, 0, 'f', 1) : QString("\xE2\x80\x94"))
            .arg(QString("%1 m").arg(elevationGain, 0, 'f', 0)));
}

void MainWindow::updateIntervals()
{
    if (!m_intervalsTable)
        return;

    const int previousRow = m_intervalsTable->currentRow();
    QSignalBlocker selectionBlocker(m_intervalsTable);

    QList<IntervalRecord> records;
    if (m_dbManager.isOpen() && m_controller->currentActivityId() > 0)
    {
        auto db = m_dbManager.database();
        IntervalRepository repo(db);
        records = repo.listIntervalsForActivity(m_controller->currentActivityId());
    }

    const auto populateIntervalsFromController = [this]()
    {
        const auto& ivs = m_controller->intervals();
        m_intervalsTable->setRowCount(static_cast<int>(ivs.size()));

        for (int row = 0; row < static_cast<int>(ivs.size()); ++row)
        {
            const auto& iv = ivs[static_cast<size_t>(row)];

            auto mkItem = [](const QString& text, Qt::Alignment align = Qt::AlignCenter)
            {
                auto* item = new QTableWidgetItem(text);
                item->setTextAlignment(align);
                return item;
            };

            auto* typeItem = mkItem(QString::fromStdString(iv.label));
            typeItem->setBackground(
                iv.label == "Work"
                    ? QBrush(QColor(60, 110, 200, 100))
                    : QBrush(QColor(60, 170, 60, 100)));
            typeItem->setData(kIntervalStartRole, iv.startSeconds);
            typeItem->setData(kIntervalEndRole, iv.endSeconds);
            m_intervalsTable->setItem(row, 0, typeItem);

            m_intervalsTable->setItem(row, 1, mkItem(fmtDur(iv.startSeconds)));
            m_intervalsTable->setItem(row, 2, mkItem(fmtDur(iv.durationSeconds)));
            m_intervalsTable->setItem(row, 3, mkItem(iv.averagePower > 0
                ? QString("%1 W").arg(iv.averagePower,   0, 'f', 0) : "\xE2\x80\x94"));
            m_intervalsTable->setItem(row, 4, mkItem(iv.normalizedPower > 0
                ? QString("%1 W").arg(iv.normalizedPower, 0, 'f', 0) : "\xE2\x80\x94"));
            const double intervalIf = (m_controller->ftp() > 0.0 && iv.normalizedPower > 0.0)
                ? (iv.normalizedPower / m_controller->ftp()) : 0.0;
            m_intervalsTable->setItem(row, 5, mkItem(intervalIf > 0.0
                ? QString::number(intervalIf, 'f', 2) : "\xE2\x80\x94"));
            m_intervalsTable->setItem(row, 6, mkItem(iv.maximumPower > 0
                ? QString("%1 W").arg(iv.maximumPower,   0, 'f', 0) : "\xE2\x80\x94"));
            m_intervalsTable->setItem(row, 7, mkItem(iv.averageHeartRate > 0
                ? QString("%1 bpm").arg(iv.averageHeartRate, 0, 'f', 0) : "\xE2\x80\x94"));
        }
    };

    const auto populateIntervalsFromRecords = [this, &records]()
    {
        m_intervalsTable->setRowCount(records.size());
        for (int row = 0; row < records.size(); ++row)
        {
            const IntervalRecord& r = records[row];
            auto* typeItem = new QTableWidgetItem(r.type);
            typeItem->setTextAlignment(Qt::AlignCenter);
            typeItem->setData(kIntervalStartRole, static_cast<double>(r.startSample));
            typeItem->setData(kIntervalEndRole, static_cast<double>(r.endSample));
            m_intervalsTable->setItem(row, 0, typeItem);
            m_intervalsTable->setItem(row, 1, new QTableWidgetItem(fmtDur(r.startSample)));
            m_intervalsTable->setItem(row, 2, new QTableWidgetItem(fmtDur(std::max(0, r.endSample - r.startSample))));
            m_intervalsTable->setItem(row, 3, new QTableWidgetItem(r.avgPower > 0 ? QString("%1 W").arg(r.avgPower, 0, 'f', 0) : "\xE2\x80\x94"));
            m_intervalsTable->setItem(row, 4, new QTableWidgetItem(r.np > 0 ? QString("%1 W").arg(r.np, 0, 'f', 0) : "\xE2\x80\x94"));
            const double intervalIf = (m_controller->ftp() > 0.0 && r.np > 0.0)
                ? (r.np / m_controller->ftp()) : 0.0;
            m_intervalsTable->setItem(row, 5, new QTableWidgetItem(intervalIf > 0.0 ? QString::number(intervalIf, 'f', 2) : "\xE2\x80\x94"));
            m_intervalsTable->setItem(row, 6, new QTableWidgetItem("\xE2\x80\x94"));
            m_intervalsTable->setItem(row, 7, new QTableWidgetItem(r.avgHr > 0 ? QString("%1 bpm").arg(r.avgHr, 0, 'f', 0) : "\xE2\x80\x94"));
        }
    };

    if (records.isEmpty())
        populateIntervalsFromController();
    else
        populateIntervalsFromRecords();

    m_intervalsTable->resizeColumnsToContents();
    finalizeIntervalSelectionAndSummary(previousRow);

    updateLapsSummaryFromCurrentActivity();
    updateActivityNotesFromCurrentActivity();
}

void MainWindow::updateLapsSummaryFromCurrentActivity()
{
    if (!m_lapsTable)
        return;

    m_lapsTable->setRowCount(1);
    m_lapsTable->setItem(0, 0, new QTableWidgetItem("1"));
    m_lapsTable->setItem(0, 1, new QTableWidgetItem("0:00"));
    m_lapsTable->setItem(0, 2, new QTableWidgetItem(fmtDur(m_controller->statistics().durationSeconds)));
    m_lapsTable->resizeColumnsToContents();
}

void MainWindow::finalizeIntervalSelectionAndSummary(int previousRow)
{
    if (!m_intervalsTable)
        return;

    if (m_intervalsTable->rowCount() > 0)
    {
        const int targetRow = std::clamp(previousRow >= 0 ? previousRow : 0, 0, m_intervalsTable->rowCount() - 1);
        m_intervalsTable->setCurrentCell(targetRow, 0);
    }

    updateIntervalRowStyles();
    if (m_intervalsTable->currentRow() >= 0)
        onIntervalSelectionChanged();
    else
        clearIntervalSummary();
}

void MainWindow::updateActivityNotesFromCurrentActivity()
{
    if (!m_activityNotesView || !m_dbManager.isOpen() || m_controller->currentActivityId() <= 0)
        return;

    auto db = m_dbManager.database();
    ActivityRepository repo(db);
    const Activity activity = repo.getActivity(m_controller->currentActivityId());

    const QDateTime activityDateUtc = QDateTime::fromString(activity.startTime, Qt::ISODate);
    const QDateTime importDateUtc = QDateTime::fromString(activity.importedAt, Qt::ISODate);
    const QString activityDateText = activityDateUtc.isValid()
        ? DateFormatter::formatDateTime(activityDateUtc.toLocalTime())
        : QStringLiteral("-");
    const QString importDateText = importDateUtc.isValid()
        ? DateFormatter::formatDateTime(importDateUtc.toLocalTime())
        : QStringLiteral("-");

    m_activityNotesView->setPlainText(
        QString("Activity Date: %1\nImported: %2\n\nNotes:\n%3\n\nWeather: %4\nTemperature: %5 C\nRPE: %6\nFatigue: %7\nSleep: %8 h\nBike: %9\nEquipment: %10")
            .arg(activityDateText)
            .arg(importDateText)
            .arg(activity.notes)
            .arg(activity.weather)
            .arg(activity.temperature, 0, 'f', 1)
            .arg(activity.rpe)
            .arg(activity.fatigue)
            .arg(activity.sleepHours, 0, 'f', 1)
            .arg(activity.bike)
            .arg(activity.equipment));
}
