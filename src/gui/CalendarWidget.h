// SPDX-License-Identifier: GPL-3

/**
 * @file CalendarWidget.h
 * @brief User interface component for CalendarWidget.
 *
 * Defines dialogs, widgets, controllers, and UI workflows used by the FitlyzerC desktop application.
 *
 * Responsibilities:
 * - Provide interactive user interface behavior and presentation
 *
 * @author Lars EBERHART
 */

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
    /**
     * @brief Constructs calendar widget.
     * @param parent Parent widget.
     */
    explicit CalendarWidget(QWidget* parent = nullptr);

    /**
     * @brief Sets database manager for data access.
     * @param dbManager Database manager.
     */
    void setDatabaseManager(DatabaseManager* dbManager);

    /**
     * @brief Sets athlete to display.
     * @param athleteId Athlete ID.
     */
    void setAthleteId(int athleteId);

    /**
     * @brief Refreshes calendar display.
     */
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