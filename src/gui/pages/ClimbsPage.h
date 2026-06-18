// SPDX-License-Identifier: GPL-3

#pragma once

#include "BasePage.h"

class QWidget;

/**
 * @brief Top-level page for climb detection and analysis.
 *
 * Hosts the climb altitude chart (with boundary-editing enabled),
 * the climbs table, and the per-climb statistics summary.
 *
 * Climb detection parameters (minimum length, gain, gradient, dip
 * settings, smoothing) are displayed in a toolbar at the top of this
 * page.  They will be moved to Settings in Phase 11.
 *
 * Toolbar actions: Add, Edit, Merge, Delete, Undo, Redo.
 */
class ClimbsPage : public BasePage
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the Climbs page.
     * @param climbParamsBar   Widget containing climb detection parameter controls.
     * @param contentWidget    Pre-assembled climb charts + table widget.
     * @param parent           Parent widget.
     */
    explicit ClimbsPage(QWidget* climbParamsBar,
                        QWidget* contentWidget,
                        QWidget* parent = nullptr);
};
