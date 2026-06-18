// SPDX-License-Identifier: GPL-3

#pragma once

#include "BasePage.h"

class ActivityBrowser;
class QAction;
class QToolBar;

/**
 * @brief Top-level page for activity browsing, search, and management.
 *
 * Hosts the ActivityBrowser widget and a context toolbar with:
 *   - Import:   open file dialog to import FIT files
 *   - Analyze:  dropdown with Detect Climbs, Detect Intervals,
 *               and Recalculate Metrics
 *   - Edit:     open activity properties dialog
 *   - Export:   export selected activity (future)
 *
 * The toolbar actions emit signals that MainWindow connects to.
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

    /// @brief Returns the page toolbar (used by MainWindow to enable/disable actions).
    QToolBar* toolbar() const { return m_toolbar; }

signals:
    /// @brief Emitted when the user requests an import.
    void importRequested();

    /// @brief Emitted when the user requests climb detection.
    void detectClimbsRequested();

    /// @brief Emitted when the user requests interval detection.
    void detectIntervalsRequested();

    /// @brief Emitted when the user requests metric recalculation.
    void recalculateMetricsRequested();

    /// @brief Emitted when the user requests the activity properties dialog.
    void editActivityRequested();

private:
    QToolBar* m_toolbar = nullptr;
};
