// SPDX-License-Identifier: GPL-3

#pragma once

#include "BasePage.h"

class CalendarWidget;

/**
 * @brief Top-level page for activity and workout planning.
 *
 * Keeps the calendar focused on planning:
 *   - Activity calendar browsing
 *   - Planned workout scheduling
 *
 * No ride-analysis widgets appear on this page.
 */
class CalendarPage : public BasePage
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the Calendar page.
     * @param calendarWidget The CalendarWidget (ownership transfers here).
     * @param parent         Parent widget.
     */
    explicit CalendarPage(CalendarWidget* calendarWidget, QWidget* parent = nullptr);
};
