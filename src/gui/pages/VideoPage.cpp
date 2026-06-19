// SPDX-License-Identifier: GPL-3

#include "VideoPage.h"

#include <QAction>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QToolBar>
#include <QVBoxLayout>

/**
 * @brief Constructs the Video page.
 *
 * Shows a contextual toolbar and a three-section layout:
 * Preview, Render Settings, and Output Settings.
 */
VideoPage::VideoPage(QWidget* parent)
    : BasePage(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // Context toolbar
    auto* toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->setIconSize(QSize(16, 16));

    auto* previewAct = new QAction("Preview", this);
    previewAct->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    previewAct->setToolTip("Open preview workflow");
    connect(previewAct, &QAction::triggered, this, &VideoPage::previewRequested);
    toolbar->addAction(previewAct);

    auto* renderAct = new QAction("Render", this);
    renderAct->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    renderAct->setToolTip("Open render workflow");
    connect(renderAct, &QAction::triggered, this, &VideoPage::renderRequested);
    toolbar->addAction(renderAct);

    auto* exportAct = new QAction("Export", this);
    exportAct->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    exportAct->setToolTip("Open export workflow");
    connect(exportAct, &QAction::triggered, this, &VideoPage::exportRequested);
    toolbar->addAction(exportAct);

    layout->addWidget(toolbar);

    auto makeCard = [this](const QString& title, const QString& body)
    {
        auto* card = new QFrame(this);
        card->setFrameShape(QFrame::StyledPanel);
        card->setStyleSheet(
            "QFrame {"
            "  background: #ffffff;"
            "  border: 1px solid #e2e8f0;"
            "  border-radius: 8px;"
            "}");

        auto* vl = new QVBoxLayout(card);
        vl->setContentsMargins(14, 12, 14, 12);
        vl->setSpacing(6);

        auto* heading = new QLabel(title, card);
        heading->setStyleSheet("font-size: 15px; font-weight: 700; color: #0f172a;");
        vl->addWidget(heading);

        auto* text = new QLabel(body, card);
        text->setWordWrap(true);
        text->setStyleSheet("font-size: 13px; color: #475569;");
        vl->addWidget(text);

        return card;
    };

    auto* previewCard = makeCard(
        "Preview",
        "Scrub and validate overlays before rendering. Use Preview to launch the existing workflow.");
    auto* renderCard = makeCard(
        "Render Settings",
        "Resolution, frame rate, and overlay composition are configured in the render flow.");
    auto* outputCard = makeCard(
        "Output Settings",
        "Choose destination, codec profile, and export naming in the export flow.");

    layout->addWidget(previewCard);

    auto* row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(6);
    row->addWidget(renderCard, 1);
    row->addWidget(outputCard, 1);
    layout->addLayout(row, 1);
}
