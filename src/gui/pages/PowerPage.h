// SPDX-License-Identifier: GPL-3

#pragma once

#include "BasePage.h"

class QTabWidget;

/**
 * @brief Top-level page for power analysis.
 *
 * Contains three sub-views accessed via an internal QTabWidget:
 *   - Power Curve  (PDC)
 *   - Histogram
 *   - Zones
 *
 * Future additions: Critical Power, W', Power Profile, Power Trends.
 */
class PowerPage : public BasePage
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the Power page.
     * @param powerSubTabs Pre-built QTabWidget containing Zones, Histogram,
     *                     and Power Curve sub-tabs (ownership transfers here).
     * @param parent       Parent widget.
     */
    explicit PowerPage(QTabWidget* powerSubTabs, QWidget* parent = nullptr);
};
