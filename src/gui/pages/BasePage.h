// SPDX-License-Identifier: GPL-3

#pragma once

#include <QWidget>

/**
 * @brief Abstract base class for all top-level page widgets.
 *
 * Each page corresponds to one entry in NavigationSidebar::Page and occupies
 * one slot in the main window's QStackedWidget (m_pageStack).
 *
 * Subclasses are responsible for building their own UI in the constructor.
 * They may optionally override onActivated() to refresh data when the user
 * navigates to the page.
 *
 * ## Adding a new feature as a new page
 *
 * 1. Create MyFeaturePage : public BasePage in src/gui/pages/.
 * 2. Add the corresponding entry to NavigationSidebar::Page (append at end).
 * 3. Add the matching item label to NavigationSidebar::NavigationSidebar().
 * 4. Append `m_pageStack->addWidget(new MyFeaturePage(...))` in buildUI()
 *    at index == static_cast<int>(Page::MyFeature).
 * 5. Update NavigationController::restoreNavigationState() to clamp to the
 *    new last valid page value.
 * 6. Add the new source files to CMakeLists.txt.
 *
 * MainWindow does NOT need to be modified to add new page-scoped features.
 * Inter-page navigation (e.g. selecting an activity opens Charts) is done
 * via NavigationController::navigateTo() or NavigationSidebar::setCurrentPage().
 */
class BasePage : public QWidget
{
    Q_OBJECT

public:
    explicit BasePage(QWidget* parent = nullptr) : QWidget(parent) {}

    /**
     * @brief Called by MainWindow when this page becomes the active page.
     *
     * Override to refresh stale data or update UI state.  The default
     * implementation does nothing.
     */
    virtual void onActivated() {}
};
