// SPDX-License-Identifier: GPL-3


#include "WelcomeWidget.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

// ── Helpers ──────────────────────────────────────────────────────────────────

namespace
{
/// Creates a styled card frame for dashboard sections.
QFrame* makeCard(QWidget* parent)
{
    auto* card = new QFrame(parent);
    card->setStyleSheet(
        "QFrame {"
        "  background: #ffffff;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 10px;"
        "}");
    return card;
}

/// Creates a section heading label.
QLabel* makeHeading(const QString& text, QWidget* parent)
{
    auto* lbl = new QLabel(text, parent);
    lbl->setStyleSheet("font-size: 13px; font-weight: 700; color: #475569; "
                       "text-transform: uppercase; letter-spacing: 0.05em;");
    return lbl;
}

/// Creates a primary action button.
QPushButton* makePrimaryBtn(const QString& text, QWidget* parent)
{
    auto* btn = new QPushButton(text, parent);
    btn->setMinimumHeight(36);
    btn->setStyleSheet(
        "QPushButton {"
        "  background: #1d4ed8; color: white; border: none; border-radius: 8px;"
        "  padding: 6px 14px; font-weight: 600; font-size: 13px;"
        "}"
        "QPushButton:hover { background: #1e40af; }"
        "QPushButton:pressed { background: #1e3a8a; }");
    return btn;
}

/// Creates a secondary action button.
QPushButton* makeSecondaryBtn(const QString& text, QWidget* parent)
{
    auto* btn = new QPushButton(text, parent);
    btn->setMinimumHeight(36);
    btn->setStyleSheet(
        "QPushButton {"
        "  background: #f1f5f9; color: #0f172a; border: 1px solid #cbd5e1;"
        "  border-radius: 8px; padding: 6px 14px; font-size: 13px;"
        "}"
        "QPushButton:hover { background: #e2e8f0; }");
    return btn;
}
} // namespace

// ── WelcomeWidget ─────────────────────────────────────────────────────────────

WelcomeWidget::WelcomeWidget(QWidget* parent)
    : QWidget(parent)
{
    // Root gradient background
    setStyleSheet(
        "WelcomeWidget {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "    stop:0 #f8fafc, stop:1 #eef2ff);"
        "}");

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 24, 24, 24);
    rootLayout->setSpacing(20);

    // ── App title ─────────────────────────────────────────────────────────
    auto* titleLabel = new QLabel("FitlyzerC", this);
    titleLabel->setStyleSheet(
        "font-size: 32px; font-weight: 800; color: #0f172a;");
    titleLabel->setAlignment(Qt::AlignCenter);
    rootLayout->addWidget(titleLabel);

    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setWordWrap(true);
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_subtitleLabel->setStyleSheet("font-size: 14px; color: #64748b;");
    rootLayout->addWidget(m_subtitleLabel);

    // ── Two-column row: Quick Actions | Recent Activities ─────────────────
    auto* columnsLayout = new QHBoxLayout;
    columnsLayout->setSpacing(16);

    // Left: Quick Actions
    {
        auto* card = makeCard(this);
        auto* vl   = new QVBoxLayout(card);
        vl->setContentsMargins(16, 16, 16, 16);
        vl->setSpacing(8);

        vl->addWidget(makeHeading("Quick Actions", card));

        m_importButton   = makePrimaryBtn("Import FIT Files", card);
        m_openDbButton   = makeSecondaryBtn("Open Database", card);
        m_createDbButton = makeSecondaryBtn("Create Database", card);

        vl->addWidget(m_importButton);
        vl->addWidget(m_openDbButton);
        vl->addWidget(m_createDbButton);
        vl->addStretch(1);

        columnsLayout->addWidget(card, 1);
    }

    // Right: Recent Activities (placeholder)
    {
        auto* card = makeCard(this);
        auto* vl   = new QVBoxLayout(card);
        vl->setContentsMargins(16, 16, 16, 16);
        vl->setSpacing(8);

        vl->addWidget(makeHeading("Recent Activities", card));

        auto* placeholder = new QLabel(
            "Your recently imported activities\nwill appear here.", card);
        placeholder->setAlignment(Qt::AlignCenter);
        placeholder->setWordWrap(true);
        placeholder->setStyleSheet("color: #94a3b8; font-size: 13px;");
        vl->addWidget(placeholder, 1);

        columnsLayout->addWidget(card, 2);
    }

    rootLayout->addLayout(columnsLayout, 1);

    // ── Sync Status (future: Garmin Connect / Strava) ────────────────────
    {
        auto* card = makeCard(this);
        auto* vl   = new QVBoxLayout(card);
        vl->setContentsMargins(16, 12, 16, 12);
        vl->setSpacing(4);

        vl->addWidget(makeHeading("Sync Status", card));

        auto makeSyncRow = [&vl, card](const QString& name)
        {
            auto* row = new QHBoxLayout;
            auto* nameLabel = new QLabel(name, card);
            nameLabel->setStyleSheet("font-size: 13px; color: #334155;");
            auto* statusLabel = new QLabel("Not connected — coming in a future update", card);
            statusLabel->setStyleSheet("font-size: 12px; color: #94a3b8;");
            row->addWidget(nameLabel);
            row->addStretch(1);
            row->addWidget(statusLabel);
            vl->addLayout(row);
        };
        makeSyncRow("Garmin Connect");
        makeSyncRow("Strava");

        rootLayout->addWidget(card);
    }

    // ── Connect signals ───────────────────────────────────────────────────
    connect(m_importButton,   &QPushButton::clicked, this, &WelcomeWidget::importRequested);
    connect(m_openDbButton,   &QPushButton::clicked, this, &WelcomeWidget::openDatabaseRequested);
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
            "Get started: create or open a database, then import your FIT files.");
    }
    else
    {
        m_subtitleLabel->setText(
            "No activities found. Import a FIT file or open a different database.");
    }
}