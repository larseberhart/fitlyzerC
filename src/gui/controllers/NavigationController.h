// SPDX-License-Identifier: GPL-3

#pragma once

#include <QObject>
#include <memory>

class QTabWidget;
class NavigationSidebar;

/**
 * @brief Manages main window navigation and view switching.
 *
 * Handles:
 * - Sidebar navigation
 * - Page switching (tabs)
 * - View activation
 * - Current view tracking
 * - Navigation state persistence
 */
class NavigationController : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Enumeration of main window tabs.
     */
    enum TabIndex
    {
        TabActivities = 0,   ///< Activity browser tab
        TabAnalysis = 1,     ///< Analysis (charts, intervals, climbs) tab
        TabWorkouts = 2      ///< Workouts tab
    };

    /**
     * @brief Enumeration of analysis sub-tabs.
     */
    enum AnalysisTabIndex
    {
        AnalysisTabActivity = 0,    ///< Activity details tab
        AnalysisTabZones = 1,       ///< Power zones tab
        AnalysisTabHistogram = 2,   ///< Power histogram tab
        AnalysisTabPowerCurve = 3,  ///< Power duration curve tab
        AnalysisTabCalendar = 4,    ///< Calendar tab
        AnalysisTabFitness = 5,     ///< Fitness chart tab
        AnalysisTabClimbing = 6     ///< Climbing analysis tab
    };

    /**
     * @brief Constructs the navigation controller.
     * @param parent Parent object.
     */
    explicit NavigationController(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~NavigationController() override;

    /**
     * @brief Sets the main tab widget.
     * @param tabWidget Pointer to the main QTabWidget.
     */
    void setMainTabWidget(QTabWidget* tabWidget);

    /**
     * @brief Sets the analysis tab widget.
     * @param tabWidget Pointer to the analysis QTabWidget.
     */
    void setAnalysisTabWidget(QTabWidget* tabWidget);

    /**
     * @brief Sets the navigation sidebar.
     * @param sidebar Pointer to NavigationSidebar widget.
     */
    void setNavigationSidebar(NavigationSidebar* sidebar);

    /**
     * @brief Switches to a main window tab.
     * @param tabIndex Index of tab to switch to.
     */
    void switchToMainTab(TabIndex tabIndex);

    /**
     * @brief Switches to an analysis sub-tab.
     * @param tabIndex Index of analysis tab to switch to.
     */
    void switchToAnalysisTab(AnalysisTabIndex tabIndex);

    /**
     * @brief Gets the currently active main tab.
     * @return Index of active main tab.
     */
    TabIndex currentMainTab() const;

    /**
     * @brief Gets the currently active analysis tab.
     * @return Index of active analysis tab.
     */
    AnalysisTabIndex currentAnalysisTab() const;

    /**
     * @brief Saves current navigation state to settings.
     */
    void saveNavigationState();

    /**
     * @brief Restores navigation state from settings.
     */
    void restoreNavigationState();

signals:
    /// @brief Emitted when main tab changes.
    void mainTabChanged(int tabIndex);

    /// @brief Emitted when analysis sub-tab changes.
    void analysisTabChanged(int tabIndex);

    /// @brief Emitted when navigation state is restored.
    void navigationStateRestored();

private:
    /// @brief Helper to connect tab change signals.
    void connectTabSignals();

    QTabWidget* m_mainTabWidget = nullptr;
    QTabWidget* m_analysisTabWidget = nullptr;
    NavigationSidebar* m_sidebar = nullptr;

    int m_currentMainTabIndex = 0;
    int m_currentAnalysisTabIndex = 0;
};
