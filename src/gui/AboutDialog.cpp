// SPDX-License-Identifier: GPL-3


#include "AboutDialog.h"

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("About FitlyzerC");
    setMinimumWidth(450);

    auto* layout = new QVBoxLayout(this);

    // ── Title ─────────────────────────────────────────────────────────────
    auto* titleLabel = new QLabel("FitlyzerC", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin-bottom: 8px;");
    layout->addWidget(titleLabel);

    // ── Description ───────────────────────────────────────────────────────
    auto* descLabel = new QLabel(
        "FitlyzerC is a desktop application for analyzing cycling workouts and fitness data from "
        "FIT format files. It provides comprehensive training analytics, power curve analysis, "
        "and performance tracking for cyclists.",
        this);
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);
    layout->addSpacing(12);

    // ── Contact & Links ────────────────────────────────────────────────────
    auto* contactLabel = new QLabel("Contact & Links:", this);
    contactLabel->setStyleSheet("font-weight: bold; margin-top: 8px;");
    layout->addWidget(contactLabel);

    auto* emailLabel = new QLabel(
        "Email: <a href=\"mailto:email@larseberhart.com\">email@larseberhart.com</a>",
        this);
    emailLabel->setOpenExternalLinks(true);
    layout->addWidget(emailLabel);

    auto* gitHubLabel = new QLabel(
        "GitHub: <a href=\"https://github.com/larseberhart/fitlyzerC\">https://github.com/larseberhart/fitlyzerC</a>",
        this);
    gitHubLabel->setOpenExternalLinks(true);
    layout->addWidget(gitHubLabel);

    layout->addSpacing(8);
    layout->addStretch();

    // ── Close button ───────────────────────────────────────────────────────
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::accept);
    layout->addWidget(btnBox);
}