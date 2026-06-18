// SPDX-License-Identifier: GPL-3

#include "PowerPage.h"

#include <QTabWidget>
#include <QVBoxLayout>

/**
 * @brief Constructs the Power page.
 *
 * Accepts the pre-built power sub-tab widget (Zones / Histogram / Power
 * Curve) and embeds it in this page's layout.
 *
 * @param powerSubTabs QTabWidget containing the power analysis sub-views.
 * @param parent       Parent widget.
 */
PowerPage::PowerPage(QTabWidget* powerSubTabs, QWidget* parent)
    : BasePage(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    if (powerSubTabs)
        layout->addWidget(powerSubTabs);
}
