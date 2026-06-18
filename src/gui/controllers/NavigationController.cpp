// SPDX-License-Identifier: GPL-3

#include "NavigationController.h"

#include <QTabWidget>
#include <QSettings>
#include "../NavigationSidebar.h"

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
 * @brief Sets the main tab widget.
 * @param tabWidget Pointer to the main QTabWidget.
 */
void NavigationController::setMainTabWidget(QTabWidget* tabWidget)
{
    m_mainTabWidget = tabWidget;
    connectTabSignals();
}

/**
 * @brief Sets the analysis tab widget.
 * @param tabWidget Pointer to the analysis QTabWidget.
 */
void NavigationController::setAnalysisTabWidget(QTabWidget* tabWidget)
{
    m_analysisTabWidget = tabWidget;
    connectTabSignals();
}

/**
 * @brief Sets the navigation sidebar.
 * @param sidebar Pointer to NavigationSidebar widget.
 */
void NavigationController::setNavigationSidebar(NavigationSidebar* sidebar)
{
    m_sidebar = sidebar;
}

/**
 * @brief Switches to a main window tab.
 * @param tabIndex Index of tab to switch to.
 */
void NavigationController::switchToMainTab(TabIndex tabIndex)
{
    if (!m_mainTabWidget)
        return;

    m_currentMainTabIndex = static_cast<int>(tabIndex);
    m_mainTabWidget->setCurrentIndex(static_cast<int>(tabIndex));
    emit mainTabChanged(static_cast<int>(tabIndex));
}

/**
 * @brief Switches to an analysis sub-tab.
 * @param tabIndex Index of analysis tab to switch to.
 */
void NavigationController::switchToAnalysisTab(AnalysisTabIndex tabIndex)
{
    if (!m_analysisTabWidget)
        return;

    m_currentAnalysisTabIndex = static_cast<int>(tabIndex);
    m_analysisTabWidget->setCurrentIndex(static_cast<int>(tabIndex));
    emit analysisTabChanged(static_cast<int>(tabIndex));
}

/**
 * @brief Gets the currently active main tab.
 * @return Index of active main tab.
 */
NavigationController::TabIndex NavigationController::currentMainTab() const
{
    if (!m_mainTabWidget)
        return TabActivities;

    return static_cast<TabIndex>(m_mainTabWidget->currentIndex());
}

/**
 * @brief Gets the currently active analysis tab.
 * @return Index of active analysis tab.
 */
NavigationController::AnalysisTabIndex NavigationController::currentAnalysisTab() const
{
    if (!m_analysisTabWidget)
        return AnalysisTabActivity;

    return static_cast<AnalysisTabIndex>(m_analysisTabWidget->currentIndex());
}

/**
 * @brief Saves current navigation state to settings.
 */
void NavigationController::saveNavigationState()
{
    QSettings settings("Fitlyzer", "FitlyzerC");
    settings.setValue("activeTab", m_currentMainTabIndex);
    settings.setValue("analysisTab", m_currentAnalysisTabIndex);
}

/**
 * @brief Restores navigation state from settings.
 */
void NavigationController::restoreNavigationState()
{
    QSettings settings("Fitlyzer", "FitlyzerC");
    
    int mainTab = settings.value("activeTab", static_cast<int>(TabActivities)).toInt();
    int analysisTab = settings.value("analysisTab", static_cast<int>(AnalysisTabActivity)).toInt();

    if (mainTab >= TabActivities && mainTab <= TabWorkouts)
        switchToMainTab(static_cast<TabIndex>(mainTab));

    if (analysisTab >= AnalysisTabActivity && analysisTab <= AnalysisTabClimbing)
        switchToAnalysisTab(static_cast<AnalysisTabIndex>(analysisTab));

    emit navigationStateRestored();
}

/**
 * @brief Helper to connect tab change signals.
 */
void NavigationController::connectTabSignals()
{
    if (m_mainTabWidget)
    {
        connect(m_mainTabWidget, qOverload<int>(&QTabWidget::currentChanged),
                this, [this](int index)
        {
            m_currentMainTabIndex = index;
            emit mainTabChanged(index);
        });
    }

    if (m_analysisTabWidget)
    {
        connect(m_analysisTabWidget, qOverload<int>(&QTabWidget::currentChanged),
                this, [this](int index)
        {
            m_currentAnalysisTabIndex = index;
            emit analysisTabChanged(index);
        });
    }
}
