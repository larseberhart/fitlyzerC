// SPDX-License-Identifier: GPL-3

/**
 * @file CalendarWidget.cpp
 * @brief User interface component for CalendarWidget.
 *
 * Defines dialogs, widgets, controllers, and UI workflows used by the FitlyzerC desktop application.
 *
 * Responsibilities:
 * - Provide interactive user interface behavior and presentation
 *
 * @author Lars EBERHART
 */

#include "CalendarWidget.h"

#include "PlannedWorkoutDialog.h"
#include "core/settings/DateFormatter.h"
#include "database/DatabaseManager.h"

#include <QCalendarWidget>
#include <QComboBox>
#include <QDateEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlQuery>
#include <QTextCharFormat>
#include <QVBoxLayout>

CalendarWidget::CalendarWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    auto* top = new QHBoxLayout;
    top->addWidget(new QLabel("Color by:", this));

    m_colorByCombo = new QComboBox(this);
    m_colorByCombo->addItem("TSS");
    m_colorByCombo->addItem("Duration");
    m_colorByCombo->addItem("Workout Type");
    top->addWidget(m_colorByCombo);

    m_addButton = new QPushButton("Add", this);
    m_editButton = new QPushButton("Edit", this);
    m_deleteButton = new QPushButton("Delete", this);
    top->addWidget(m_addButton);
    top->addWidget(m_editButton);
    top->addWidget(m_deleteButton);
    top->addStretch(1);

    m_calendar = new QCalendarWidget(this);
    m_calendar->setGridVisible(true);

    m_dayList = new QListWidget(this);
    m_dayList->addItem("Planned workouts and completed activities will appear here.");

    layout->addLayout(top);
    layout->addWidget(m_calendar);
    layout->addWidget(m_dayList);

    connect(m_calendar, &QCalendarWidget::selectionChanged,
            this, &CalendarWidget::onDateChanged);
    connect(m_calendar, &QCalendarWidget::currentPageChanged,
            this, [this](int, int) { updateMonthFormatting(); });
    connect(m_colorByCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &CalendarWidget::recolorMonth);
    connect(m_addButton, &QPushButton::clicked,
            this, &CalendarWidget::addPlannedWorkout);
    connect(m_editButton, &QPushButton::clicked,
            this, &CalendarWidget::editSelectedWorkout);
    connect(m_deleteButton, &QPushButton::clicked,
            this, &CalendarWidget::deleteSelectedWorkout);
}

void CalendarWidget::setDatabaseManager(DatabaseManager* dbManager)
{
    m_dbManager = dbManager;
    refresh();
}

void CalendarWidget::setAthleteId(int athleteId)
{
    m_athleteId = athleteId;
    refresh();
}

void CalendarWidget::refresh()
{
    if (!m_calendar)
        return;
    reloadDayList(m_calendar->selectedDate());
    updateMonthFormatting();
}

void CalendarWidget::onDateChanged()
{
    if (!m_calendar)
        return;
    reloadDayList(m_calendar->selectedDate());
}

void CalendarWidget::addPlannedWorkout()
{
    if (!m_dbManager || !m_dbManager->isOpen() || m_athleteId <= 0)
        return;

    PlannedWorkoutDraft draft;
    draft.date = m_calendar ? m_calendar->selectedDate() : QDate::currentDate();
    PlannedWorkoutDialog dlg(draft, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    const PlannedWorkoutDraft saved = dlg.draft();
    if (!saved.date.isValid() || saved.title.isEmpty())
        return;

    QSqlQuery q(m_dbManager->database());
    q.prepare(
        "INSERT INTO planned_workouts(athlete_id, workout_date, title, type, duration_sec, target_tss, notes) "
        "VALUES(:athlete_id, :workout_date, :title, :type, :duration_sec, :target_tss, :notes)");
    q.bindValue(":athlete_id", m_athleteId);
    q.bindValue(":workout_date", DateFormatter::toIsoDate(saved.date));
    q.bindValue(":title", saved.title);
    q.bindValue(":type", saved.type);
    q.bindValue(":duration_sec", saved.durationSeconds);
    q.bindValue(":target_tss", saved.targetTss);
    q.bindValue(":notes", saved.notes);
    if (!q.exec())
    {
        QMessageBox::critical(this, "Calendar", "Failed to add planned workout.");
        return;
    }

    refresh();
}

void CalendarWidget::editSelectedWorkout()
{
    if (!m_dbManager || !m_dbManager->isOpen() || !m_dayList)
        return;

    QListWidgetItem* item = m_dayList->currentItem();
    if (!item)
        return;

    const int id = item->data(Qt::UserRole).toInt();
    if (id <= 0)
        return;

    QSqlQuery q(m_dbManager->database());
    q.prepare(
        "SELECT workout_date, title, type, duration_sec, target_tss, notes "
        "FROM planned_workouts WHERE id=:id");
    q.bindValue(":id", id);
    if (!q.exec() || !q.next())
        return;

    PlannedWorkoutDraft draft;
    draft.id = id;
    draft.date = QDate::fromString(q.value(0).toString(), Qt::ISODate);
    draft.title = q.value(1).toString();
    draft.type = q.value(2).toString();
    draft.durationSeconds = q.value(3).toDouble();
    draft.targetTss = q.value(4).toDouble();
    draft.notes = q.value(5).toString();

    PlannedWorkoutDialog dlg(draft, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    const PlannedWorkoutDraft saved = dlg.draft();
    QSqlQuery update(m_dbManager->database());
    update.prepare(
        "UPDATE planned_workouts SET workout_date=:workout_date, title=:title, type=:type, "
        "duration_sec=:duration_sec, target_tss=:target_tss, notes=:notes WHERE id=:id");
    update.bindValue(":workout_date", DateFormatter::toIsoDate(saved.date));
    update.bindValue(":title", saved.title);
    update.bindValue(":type", saved.type);
    update.bindValue(":duration_sec", saved.durationSeconds);
    update.bindValue(":target_tss", saved.targetTss);
    update.bindValue(":notes", saved.notes);
    update.bindValue(":id", id);
    if (!update.exec())
    {
        QMessageBox::critical(this, "Calendar", "Failed to update planned workout.");
        return;
    }

    refresh();
}

void CalendarWidget::deleteSelectedWorkout()
{
    if (!m_dbManager || !m_dbManager->isOpen() || !m_dayList)
        return;

    QListWidgetItem* item = m_dayList->currentItem();
    if (!item)
        return;

    const int id = item->data(Qt::UserRole).toInt();
    if (id <= 0)
        return;

    QSqlQuery q(m_dbManager->database());
    q.prepare("DELETE FROM planned_workouts WHERE id=:id");
    q.bindValue(":id", id);
    if (!q.exec())
    {
        QMessageBox::critical(this, "Calendar", "Failed to delete planned workout.");
        return;
    }

    refresh();
}

void CalendarWidget::recolorMonth()
{
    updateMonthFormatting();
}

void CalendarWidget::reloadDayList(const QDate& date)
{
    if (!m_dayList)
        return;

    m_dayList->clear();
    if (!m_dbManager || !m_dbManager->isOpen() || m_athleteId <= 0)
    {
        m_dayList->addItem("Open a database and select an athlete.");
        return;
    }

    QSqlQuery q(m_dbManager->database());
    q.prepare(
        "SELECT id, title, type, duration_sec, target_tss FROM planned_workouts "
        "WHERE athlete_id=:athlete_id AND workout_date=:workout_date ORDER BY id ASC");
    q.bindValue(":athlete_id", m_athleteId);
    q.bindValue(":workout_date", DateFormatter::toIsoDate(date));
    q.exec();

    bool hasRows = false;
    while (q.next())
    {
        hasRows = true;
        const int id = q.value(0).toInt();
        const QString title = q.value(1).toString();
        const QString type = q.value(2).toString();
        const double durationSec = q.value(3).toDouble();
        const double tss = q.value(4).toDouble();
        const QString text = QString("%1 (%2, %3 h, TSS %4)")
            .arg(title)
            .arg(type.isEmpty() ? QStringLiteral("Planned") : type)
            .arg(durationSec / 3600.0, 0, 'f', 2)
            .arg(tss, 0, 'f', 0);
        auto* item = new QListWidgetItem(text, m_dayList);
        item->setData(Qt::UserRole, id);
    }

    if (!hasRows)
        m_dayList->addItem("No planned workouts for this day.");
}

void CalendarWidget::updateMonthFormatting()
{
    if (!m_calendar)
        return;

    const QTextCharFormat defaultFormat;
    const QDate shown = m_calendar->selectedDate().isValid() ? m_calendar->selectedDate() : QDate::currentDate();
    const QDate monthStart(shown.year(), shown.month(), 1);
    const QDate monthEnd = monthStart.addMonths(1).addDays(-1);

    for (QDate d = monthStart; d <= monthEnd; d = d.addDays(1))
        m_calendar->setDateTextFormat(d, defaultFormat);

    if (!m_dbManager || !m_dbManager->isOpen() || m_athleteId <= 0)
        return;

    QSqlQuery q(m_dbManager->database());
    q.prepare(
        "SELECT workout_date, SUM(target_tss), SUM(duration_sec), COUNT(*), MAX(type) "
        "FROM planned_workouts "
        "WHERE athlete_id=:athlete_id AND workout_date>=:start_date AND workout_date<=:end_date "
        "GROUP BY workout_date");
    q.bindValue(":athlete_id", m_athleteId);
    q.bindValue(":start_date", DateFormatter::toIsoDate(monthStart));
    q.bindValue(":end_date", DateFormatter::toIsoDate(monthEnd));
    q.exec();

    while (q.next())
    {
        const QDate date = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        const double totalTss = q.value(1).toDouble();
        const double totalSec = q.value(2).toDouble();
        const int count = q.value(3).toInt();
        const QString type = q.value(4).toString();

        QColor bg("#dbeafe");
        const QString colorBy = m_colorByCombo ? m_colorByCombo->currentText() : QStringLiteral("TSS");
        if (colorBy == "TSS")
        {
            if (totalTss >= 200.0) bg = QColor("#991b1b");
            else if (totalTss >= 120.0) bg = QColor("#dc2626");
            else if (totalTss >= 70.0) bg = QColor("#f97316");
            else if (totalTss >= 30.0) bg = QColor("#facc15");
            else bg = QColor("#86efac");
        }
        else if (colorBy == "Duration")
        {
            const double hours = totalSec / 3600.0;
            if (hours >= 4.0) bg = QColor("#1d4ed8");
            else if (hours >= 2.0) bg = QColor("#2563eb");
            else if (hours >= 1.0) bg = QColor("#38bdf8");
            else bg = QColor("#bae6fd");
        }
        else
        {
            if (type.contains("VO2", Qt::CaseInsensitive)) bg = QColor("#ef4444");
            else if (type.contains("Threshold", Qt::CaseInsensitive)) bg = QColor("#f59e0b");
            else if (type.contains("Endurance", Qt::CaseInsensitive)) bg = QColor("#22c55e");
            else bg = QColor("#a3a3a3");
        }

        QTextCharFormat fmt;
        fmt.setBackground(bg);
        fmt.setForeground((bg.lightness() < 128) ? QColor("#f8fafc") : QColor("#111827"));
        fmt.setToolTip(QString("%1 planned workouts").arg(count));
        m_calendar->setDateTextFormat(date, fmt);
    }
}