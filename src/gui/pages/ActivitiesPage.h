// SPDX-License-Identifier: GPL-3

#pragma once

#include "BasePage.h"

class ActivityBrowser;

/**
 * @brief Top-level page for activity browsing, search, and management.
 *
 * Hosts the ActivityBrowser widget.  Import, analyse, edit, and export
 * toolbar actions are scoped to this page (Phase 3 will complete the
 * context toolbar).
 */
class ActivitiesPage : public BasePage
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the Activities page.
     * @param browser The ActivityBrowser widget (ownership transfers to this page).
     * @param parent  Parent widget.
     */
    explicit ActivitiesPage(ActivityBrowser* browser, QWidget* parent = nullptr);
};
