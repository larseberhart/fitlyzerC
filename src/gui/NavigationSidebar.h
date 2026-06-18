// SPDX-License-Identifier: GPL-3


#pragma once

#include <QWidget>

class QListWidget;
class QListWidgetItem;

/**
 * @brief Main navigation sidebar.
 *
 * Displays the top-level pages available in the application.
 * The Page enum maps 1-to-1 to list-widget rows, so values must
 * remain contiguous and start at zero.
 */
class NavigationSidebar : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Top-level application pages.
     *
     * Matches the order in which items appear in the sidebar list.
     * Future pages (Reports, Readiness, Settings) should be appended
     * at the end so that existing values are not renumbered.
     */
    enum class Page
    {
        Activities = 0, ///< Activity browser and management
        Charts,         ///< Activity charts and map
        Power,          ///< Power curve, histogram, and zones
        Intervals,      ///< Interval table and editor
        Climbs,         ///< Climb detection and analysis
        Fitness,        ///< Fitness, CTL/ATL/TSB, and FTP history
        Calendar,       ///< Activity and workout calendar
        Video           ///< Video creation and export
    };

    explicit NavigationSidebar(QWidget* parent = nullptr);

    /// @brief Selects the given page without emitting pageSelected.
    void setCurrentPage(Page page);

    /// @brief Returns the currently selected page.
    Page currentPage() const;

signals:
    /// @brief Emitted when the user selects a different page.
    void pageSelected(NavigationSidebar::Page page);

private:
    QListWidget* m_list = nullptr;
};