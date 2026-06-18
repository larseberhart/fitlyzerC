// SPDX-License-Identifier: GPL-3


#include "NavigationSidebar.h"

#include <QListWidget>
#include <QVBoxLayout>

NavigationSidebar::NavigationSidebar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("navigationSidebar");
    setStyleSheet(
        "QWidget#navigationSidebar {"
        "  background: #1e293b;"
        "}"
        "QListWidget {"
        "  background: transparent;"
        "  border: none;"
        "  outline: none;"
        "  color: #94a3b8;"
        "  font-size: 13px;"
        "  font-weight: 500;"
        "}"
        "QListWidget::item {"
        "  padding: 9px 14px;"
        "  border-radius: 6px;"
        "  margin: 1px 6px;"
        "}"
        "QListWidget::item:selected {"
        "  background: #334155;"
        "  color: #f1f5f9;"
        "}"
        "QListWidget::item:hover:!selected {"
        "  background: #273449;"
        "  color: #e2e8f0;"
        "}");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 8, 0, 8);

    m_list = new QListWidget(this);
    // Items must be added in the same order as the Page enum.
    m_list->addItem("Activities");
    m_list->addItem("Charts");
    m_list->addItem("Power");
    m_list->addItem("Intervals");
    m_list->addItem("Climbs");
    m_list->addItem("Fitness");
    m_list->addItem("Calendar");
    m_list->addItem("Video");
    m_list->setCurrentRow(static_cast<int>(Page::Activities));
    m_list->setFixedWidth(170);
    layout->addWidget(m_list);

    connect(m_list, &QListWidget::currentRowChanged, this, [this](int row)
    {
        if (row < 0)
            return;
        emit pageSelected(static_cast<Page>(row));
    });
}

void NavigationSidebar::setCurrentPage(Page page)
{
    if (!m_list)
        return;
    m_list->setCurrentRow(static_cast<int>(page));
}

NavigationSidebar::Page NavigationSidebar::currentPage() const
{
    if (!m_list)
        return Page::Activities;
    const int row = m_list->currentRow();
    return row < 0 ? Page::Activities : static_cast<Page>(row);
}