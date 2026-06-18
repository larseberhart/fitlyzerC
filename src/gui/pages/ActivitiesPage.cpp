// SPDX-License-Identifier: GPL-3

#include "ActivitiesPage.h"

#include <QAction>
#include <QMenu>
#include <QStyle>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include "../ActivityBrowser.h"

/**
 * @brief Constructs the Activities page.
 *
 * Wraps the pre-built ActivityBrowser in a page that also provides a context
 * toolbar with Import, Analyze (dropdown), and Edit actions.
 *
 * @param browser ActivityBrowser widget (ownership transfers here).
 * @param parent  Parent widget.
 */
ActivitiesPage::ActivitiesPage(ActivityBrowser* browser, QWidget* parent)
    : BasePage(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Context toolbar
    m_toolbar = new QToolBar(this);
    m_toolbar->setMovable(false);
    m_toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toolbar->setIconSize(QSize(16, 16));

    // Import
    auto* importAct = new QAction("Import", this);
    importAct->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    importAct->setToolTip("Import FIT files");
    connect(importAct, &QAction::triggered, this, &ActivitiesPage::importRequested);
    m_toolbar->addAction(importAct);

    m_toolbar->addSeparator();

    // Analyze dropdown
    auto* analyzeMenu = new QMenu(this);

    auto* detectClimbsAct = new QAction("Detect Climbs", analyzeMenu);
    connect(detectClimbsAct, &QAction::triggered,
            this, &ActivitiesPage::detectClimbsRequested);
    analyzeMenu->addAction(detectClimbsAct);

    auto* detectIntervalsAct = new QAction("Detect Intervals", analyzeMenu);
    connect(detectIntervalsAct, &QAction::triggered,
            this, &ActivitiesPage::detectIntervalsRequested);
    analyzeMenu->addAction(detectIntervalsAct);

    analyzeMenu->addSeparator();

    auto* recalcAct = new QAction("Recalculate Metrics", analyzeMenu);
    connect(recalcAct, &QAction::triggered,
            this, &ActivitiesPage::recalculateMetricsRequested);
    analyzeMenu->addAction(recalcAct);

    auto* analyzeBtn = new QToolButton(m_toolbar);
    analyzeBtn->setText("Analyze");
    analyzeBtn->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    analyzeBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    analyzeBtn->setPopupMode(QToolButton::MenuButtonPopup);
    analyzeBtn->setMenu(analyzeMenu);
    // Default click triggers Detect Climbs
    connect(analyzeBtn, &QToolButton::clicked,
            this, &ActivitiesPage::detectClimbsRequested);
    m_toolbar->addWidget(analyzeBtn);

    m_toolbar->addSeparator();

    // Edit
    auto* editAct = new QAction("Edit", this);
    editAct->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    editAct->setToolTip("Edit selected activity properties");
    connect(editAct, &QAction::triggered, this, &ActivitiesPage::editActivityRequested);
    m_toolbar->addAction(editAct);

    layout->addWidget(m_toolbar);

    // Activity browser (takes remaining height)
    if (browser)
        layout->addWidget(browser, 1);
}
