// SPDX-License-Identifier: GPL-3


#pragma once

#include <QDate>
#include <QString>
#include <QWidget>

class QCalendarWidget;
class QComboBox;
class QListWidget;
class QPushButton;
class QListWidgetItem;
class DatabaseManager;

/**
 * @brief Calendar widget for activity scheduling and display.
 *
 * Shows activities by date with customizable coloring and planned workout support.
 */
class CalendarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CalendarWidget(QWidget* parent = nullptr);

    void setDatabaseManager(DatabaseManager* dbManager);

    void setAthleteId(int athleteId);

    void refresh();

private slots:
    void onDateChanged();
    void addPlannedWorkout();
    void editSelectedWorkout();
    void deleteSelectedWorkout();
    void recolorMonth();

private:
    void reloadDayList(const QDate& date);
    void updateMonthFormatting();

private:
    QCalendarWidget* m_calendar = nullptr;
    QComboBox* m_colorByCombo = nullptr;
    QListWidget* m_dayList = nullptr;
    QPushButton* m_addButton = nullptr;
    QPushButton* m_editButton = nullptr;
    QPushButton* m_deleteButton = nullptr;

    DatabaseManager* m_dbManager = nullptr;
    int m_athleteId = -1;
};