// SPDX-License-Identifier: GPL-3


#include "NavigationSidebar.h"

#include <QListWidget>
#include <QVBoxLayout>

NavigationSidebar::NavigationSidebar(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_list = new QListWidget(this);
    m_list->addItem("Dashboard");
    m_list->addItem("Calendar");
    m_list->addItem("Activities");
    m_list->addItem("Analysis");
    m_list->addItem("Athletes");
    m_list->addItem("Settings");
    m_list->setCurrentRow(static_cast<int>(Page::Activities));
    m_list->setFixedWidth(170);
    layout->addWidget(m_list);

    connect(m_list, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0) return;
        emit pageSelected(static_cast<Page>(row));
    });
}

void NavigationSidebar::setCurrentPage(Page page)
{
    if (!m_list) return;
    m_list->setCurrentRow(static_cast<int>(page));
}

NavigationSidebar::Page NavigationSidebar::currentPage() const
{
    if (!m_list) return Page::Activities;
    const int row = m_list->currentRow();
    return row < 0 ? Page::Activities : static_cast<Page>(row);
}