// SPDX-License-Identifier: GPL-3

#include "VideoPage.h"

#include <QAction>
#include <QLabel>
#include <QStyle>
#include <QToolBar>
#include <QVBoxLayout>

/**
 * @brief Constructs the Video page.
 *
 * Shows a toolbar with a "Create Video" action and a descriptive label.
 * The full inline preview/render/export UI is a future addition.
 */
VideoPage::VideoPage(QWidget* parent)
    : BasePage(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Context toolbar
    auto* toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->setIconSize(QSize(16, 16));

    auto* createAct = new QAction("Create Video", this);
    createAct->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    createAct->setToolTip("Open video creation dialog");
    connect(createAct, &QAction::triggered, this, &VideoPage::createVideoRequested);
    toolbar->addAction(createAct);
    layout->addWidget(toolbar);

    // Placeholder body — full video preview UI is a future addition
    auto* label = new QLabel(
        "Video Creation\n\n"
        "Click \"Create Video\" to open the video export dialog.\n"
        "Inline preview and render settings coming in a future update.",
        this);
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    label->setStyleSheet("color: #64748b; font-size: 15px;");
    layout->addWidget(label, 1);
}
