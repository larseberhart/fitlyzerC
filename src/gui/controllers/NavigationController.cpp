// SPDX-License-Identifier: GPL-3

#include "NavigationController.h"

#include <QSettings>
#include <QStackedWidget>

/**
 * @brief Constructs the navigation controller.
 * @param parent Parent object.
 */
NavigationController::NavigationController(QObject* parent)
    : QObject(parent)
{
}

/**
 * @brief Destructor.
 */
NavigationController::~NavigationController() = default;

/**
 * @brief Sets the page stacked widget.
 * @param stack Pointer to the QStackedWidget that hosts one widget per page.
 */
void NavigationController::setPageStack(QStackedWidget* stack)
{
    m_pageStack = stack;
}

/**
 * @brief Sets and wires the navigation sidebar.
 *
 * Selecting a row in the sidebar immediately navigates to the corresponding
 * page in the stack.
 * @param sidebar Pointer to NavigationSidebar.
 */
void NavigationController::setNavigationSidebar(NavigationSidebar* sidebar)
{
    m_sidebar = sidebar;
    if (!m_sidebar)
        return;

    connect(m_sidebar, &NavigationSidebar::pageSelected,
            this, &NavigationController::navigateTo);
}

/**
 * @brief Navigates to the given page.
 *
 * Updates the page stack and the sidebar selection, then emits pageChanged.
 * @param page Target page.
 */
void NavigationController::navigateTo(Page page)
{
    m_currentPage = page;

    if (m_pageStack)
        m_pageStack->setCurrentIndex(static_cast<int>(page));

    if (m_sidebar)
    {
        // Block signals to avoid a re-entrant navigateTo() call.
        const QSignalBlocker blocker(m_sidebar);
        m_sidebar->setCurrentPage(page);
    }

    emit pageChanged(page);
}

/**
 * @brief Returns the currently active page.
 */
NavigationController::Page NavigationController::currentPage() const
{
    return m_currentPage;
}

/**
 * @brief Saves the active page index to QSettings.
 */
void NavigationController::saveNavigationState()
{
    QSettings settings("Fitlyzer", "FitlyzerC");
    settings.setValue("activePage", static_cast<int>(m_currentPage));
}

/**
 * @brief Restores the active page from QSettings.
 */
void NavigationController::restoreNavigationState()
{
    QSettings settings("Fitlyzer", "FitlyzerC");
    const int raw = settings.value("activePage", static_cast<int>(Page::Activities)).toInt();

    // Clamp to the valid range so stale settings cannot crash.
    const auto page = (raw >= static_cast<int>(Page::Activities) &&
                       raw <= static_cast<int>(Page::Video))
                      ? static_cast<Page>(raw)
                      : Page::Activities;

    navigateTo(page);
    emit navigationStateRestored();
}
