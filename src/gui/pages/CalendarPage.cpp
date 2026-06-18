// SPDX-License-Identifier: GPL-3

#include "CalendarPage.h"

#include <QVBoxLayout>

#include "../CalendarWidget.h"

/**
 * @brief Constructs the Calendar page.
 * @param calendarWidget CalendarWidget to embed.
 * @param parent         Parent widget.
 */
CalendarPage::CalendarPage(CalendarWidget* calendarWidget, QWidget* parent)
    : BasePage(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    if (calendarWidget)
        layout->addWidget(calendarWidget);
}
