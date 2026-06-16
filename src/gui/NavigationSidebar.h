// SPDX-License-Identifier: GPL-3


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
    enum class Page
    {
        Dashboard = 0,
        Calendar,
        Activities,
        Analysis,
        Athletes,
        Settings
    };

    explicit NavigationSidebar(QWidget* parent = nullptr);

    void setCurrentPage(Page page);

    Page currentPage() const;

signals:
    // Emitted when navigation page selection changes.
    void pageSelected(NavigationSidebar::Page page);

private:
    QListWidget* m_list = nullptr;
};