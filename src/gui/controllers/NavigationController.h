// SPDX-License-Identifier: GPL-3

#pragma once

#include <QObject>
#include <memory>

#include "../NavigationSidebar.h"

class QStackedWidget;

/**
 * @brief Manages top-level page navigation in the main window.
 *
 * Owns the relationship between:
 * - NavigationSidebar (user input)
 * - QStackedWidget     (page display)
 *
 * The legacy QTabWidget / analysis-sub-tab hierarchy has been removed.
 * Each entry in NavigationSidebar::Page corresponds to one index in the
 * page stack.  Call setPageStack() once after both the sidebar and the
 * stack are constructed, then call connectSidebar() to wire them.
 */
class NavigationController : public QObject
{
    Q_OBJECT

public:
    /// @brief Convenience alias so callers do not need to include NavigationSidebar.h.
    using Page = NavigationSidebar::Page;

    explicit NavigationController(QObject* parent = nullptr);
    ~NavigationController() override;

    /**
     * @brief Sets the stacked widget that holds one widget per Page.
     * @param stack Pointer to the QStackedWidget used as the page container.
     */
    void setPageStack(QStackedWidget* stack);

    /**
     * @brief Sets and connects the navigation sidebar.
     *
     * After this call, selecting a row in the sidebar automatically
     * switches the page stack to the corresponding index.
     * @param sidebar Pointer to the NavigationSidebar.
     */
    void setNavigationSidebar(NavigationSidebar* sidebar);

    /**
     * @brief Navigates to the given page.
     * @param page Target page.
     */
    void navigateTo(Page page);

    /// @brief Returns the currently active page.
    Page currentPage() const;

    /// @brief Saves the active page to QSettings.
    void saveNavigationState();

    /// @brief Restores the active page from QSettings.
    void restoreNavigationState();

signals:
    /// @brief Emitted whenever the active page changes.
    void pageChanged(NavigationSidebar::Page page);

    /// @brief Emitted once after restoreNavigationState() completes.
    void navigationStateRestored();

private:
    QStackedWidget*   m_pageStack = nullptr;
    NavigationSidebar* m_sidebar  = nullptr;

    Page m_currentPage = Page::Activities;
};
