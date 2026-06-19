// SPDX-License-Identifier: GPL-3

#include "ClimbsPage.h"

#include <QAction>
#include <QStyle>
#include <QToolBar>
#include <QVBoxLayout>

/**
 * @brief Constructs the Climbs page.
 *
 * Adds a climb-context toolbar above the main climb content area.
 *
 * @param contentWidget  Pre-assembled splitter with climb charts and the
 *                       recognised-climbs table.
 * @param parent         Parent widget.
 */
ClimbsPage::ClimbsPage(QWidget* contentWidget,
                       QWidget* parent)
    : BasePage(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_toolbar = new QToolBar(this);
    m_toolbar->setMovable(false);
    m_toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toolbar->setIconSize(QSize(16, 16));

    auto* addAct = new QAction("Add", this);
    addAct->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    connect(addAct, &QAction::triggered, this, &ClimbsPage::addRequested);
    m_toolbar->addAction(addAct);

    auto* editAct = new QAction("Edit", this);
    editAct->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    connect(editAct, &QAction::triggered, this, &ClimbsPage::editRequested);
    m_toolbar->addAction(editAct);

    auto* mergeAct = new QAction("Merge", this);
    mergeAct->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
    connect(mergeAct, &QAction::triggered, this, &ClimbsPage::mergeRequested);
    m_toolbar->addAction(mergeAct);

    auto* deleteAct = new QAction("Delete", this);
    deleteAct->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    connect(deleteAct, &QAction::triggered, this, &ClimbsPage::deleteRequested);
    m_toolbar->addAction(deleteAct);

    m_toolbar->addSeparator();

    auto* undoAct = new QAction("Undo", this);
    undoAct->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    connect(undoAct, &QAction::triggered, this, &ClimbsPage::undoRequested);
    m_toolbar->addAction(undoAct);

    auto* redoAct = new QAction("Redo", this);
    redoAct->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    connect(redoAct, &QAction::triggered, this, &ClimbsPage::redoRequested);
    m_toolbar->addAction(redoAct);

    layout->addWidget(m_toolbar);

    if (contentWidget)
        layout->addWidget(contentWidget, 1);
}
