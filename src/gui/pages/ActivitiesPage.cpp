// SPDX-License-Identifier: GPL-3

#include "ActivitiesPage.h"

#include <QVBoxLayout>

#include "../ActivityBrowser.h"

/**
 * @brief Constructs the Activities page.
 *
 * Wraps the pre-built ActivityBrowser in a simple full-height layout.
 * @param browser ActivityBrowser widget (ownership transfers here).
 * @param parent  Parent widget.
 */
ActivitiesPage::ActivitiesPage(ActivityBrowser* browser, QWidget* parent)
    : BasePage(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    if (browser)
        layout->addWidget(browser);
}
