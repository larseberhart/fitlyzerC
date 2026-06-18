// SPDX-License-Identifier: GPL-3

#include "ChartsPage.h"

#include <QVBoxLayout>

/**
 * @brief Constructs the Charts page.
 *
 * Lays out the color-by control bar above the main chart+map+intervals
 * content area.
 *
 * @param colorBarWidget Widget containing the "Color By" metric selector and
 *                       the color-legend strip.
 * @param contentWidget  Pre-assembled splitter widget with charts, map,
 *                       intervals, laps, and notes panels.
 * @param parent         Parent widget.
 */
ChartsPage::ChartsPage(QWidget* colorBarWidget,
                       QWidget* contentWidget,
                       QWidget* parent)
    : BasePage(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    if (colorBarWidget)
        layout->addWidget(colorBarWidget);

    if (contentWidget)
        layout->addWidget(contentWidget, 1);
}
