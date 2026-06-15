// SPDX-License-Identifier: GPL-3

/**
 * @file NavigationSidebar.h
 * @brief User interface component for NavigationSidebar.
 *
 * Defines dialogs, widgets, controllers, and UI workflows used by the FitlyzerC desktop application.
 *
 * Responsibilities:
 * - Provide interactive user interface behavior and presentation
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QWidget>

class QListWidget;
class QListWidgetItem;

/**
 * @brief Main navigation sidebar for app pages.
 */
class NavigationSidebar : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Application page enumeration.
     */
    enum class Page
    {
        Dashboard = 0,
        Calendar,
        Activities,
        Analysis,
        Athletes,
        Settings
    };

    /**
     * @brief Constructs navigation sidebar.
     * @param parent Parent widget.
     */
    explicit NavigationSidebar(QWidget* parent = nullptr);

    /**
     * @brief Sets the current page.
     * @param page Page to select.
     */
    void setCurrentPage(Page page);

    /**
     * @brief Gets the current page.
     * @return Currently selected page.
     */
    Page currentPage() const;

signals:
    /// \signal Emitted when navigation page selection changes.
    /// \param page Selected page.
    void pageSelected(NavigationSidebar::Page page);

private:
    QListWidget* m_list = nullptr;
};