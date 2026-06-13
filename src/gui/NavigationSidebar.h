#pragma once

#include <QWidget>

class QListWidget;
class QListWidgetItem;

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
    void pageSelected(NavigationSidebar::Page page);

private:
    QListWidget* m_list = nullptr;
};
