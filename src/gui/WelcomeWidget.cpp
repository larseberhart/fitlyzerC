// SPDX-License-Identifier: GPL-3

/**
 * @file WelcomeWidget.cpp
 * @brief User interface component for WelcomeWidget.
 *
 * Defines dialogs, widgets, controllers, and UI workflows used by the FitlyzerC desktop application.
 *
 * Responsibilities:
 * - Provide interactive user interface behavior and presentation
 *
 * @author Lars EBERHART
 */

#include "WelcomeWidget.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

WelcomeWidget::WelcomeWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 12, 0, 12);

    auto* card = new QFrame(this);
    card->setObjectName("welcomeCard");
    card->setStyleSheet(
        "QFrame#welcomeCard {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #f8fafc, stop:1 #eef2ff);"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 12px;"
        "}"
    );

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(28, 24, 28, 24);
    cardLayout->setSpacing(12);

    auto* title = new QLabel("Drop FIT Files Here", card);
    title->setStyleSheet("font-size: 28px; font-weight: 700; color: #0f172a;");
    title->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(title);

    m_subtitleLabel = new QLabel(card);
    m_subtitleLabel->setWordWrap(true);
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_subtitleLabel->setStyleSheet("font-size: 14px; color: #334155;");
    cardLayout->addWidget(m_subtitleLabel);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->setSpacing(10);

    m_importButton = new QPushButton("Import Activity", card);
    m_openDbButton = new QPushButton("Open Database", card);
    m_createDbButton = new QPushButton("Create Database", card);

    m_importButton->setMinimumHeight(36);
    m_openDbButton->setMinimumHeight(36);
    m_createDbButton->setMinimumHeight(36);

    m_importButton->setStyleSheet(
        "QPushButton {"
        "  background: #1d4ed8; color: white; border: none; border-radius: 8px;"
        "  padding: 8px 14px; font-weight: 600;"
        "}"
        "QPushButton:hover { background: #1e40af; }"
    );

    buttonRow->addStretch(1);
    buttonRow->addWidget(m_importButton);
    buttonRow->addWidget(m_openDbButton);
    buttonRow->addWidget(m_createDbButton);
    buttonRow->addStretch(1);

    cardLayout->addLayout(buttonRow);

    rootLayout->addStretch(1);
    rootLayout->addWidget(card);
    rootLayout->addStretch(1);

    connect(m_importButton, &QPushButton::clicked, this, &WelcomeWidget::importRequested);
    connect(m_openDbButton, &QPushButton::clicked, this, &WelcomeWidget::openDatabaseRequested);
    connect(m_createDbButton, &QPushButton::clicked, this, &WelcomeWidget::createDatabaseRequested);

    setFirstLaunch(true);
}

void WelcomeWidget::setFirstLaunch(bool firstLaunch)
{
    if (!m_subtitleLabel)
        return;

    if (firstLaunch)
    {
        m_subtitleLabel->setText(
            "Get started in seconds: drop one or more .fit files to create your first activity automatically.");
    }
    else
    {
        m_subtitleLabel->setText(
            "No activities yet for this database. Import a .fit file to begin analysis.");
    }
}