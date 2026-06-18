// SPDX-License-Identifier: GPL-3

#include "ClimbsPage.h"

#include <QVBoxLayout>

/**
 * @brief Constructs the Climbs page.
 *
 * Places the detection-parameter bar above the main climb content area.
 *
 * @param climbParamsBar Widget containing the inline climb detection controls
 *                       (length, gain, gradient, dip distance/metres,
 *                       smoothing).  These move to Settings in Phase 11.
 * @param contentWidget  Pre-assembled splitter with climb charts and the
 *                       recognised-climbs table.
 * @param parent         Parent widget.
 */
ClimbsPage::ClimbsPage(QWidget* climbParamsBar,
                       QWidget* contentWidget,
                       QWidget* parent)
    : BasePage(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    if (climbParamsBar)
        layout->addWidget(climbParamsBar);

    if (contentWidget)
        layout->addWidget(contentWidget, 1);
}
