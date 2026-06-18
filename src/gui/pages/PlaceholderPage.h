// SPDX-License-Identifier: GPL-3

#pragma once

#include "BasePage.h"

#include <QLabel>
#include <QVBoxLayout>

/**
 * @brief A placeholder page shown for features not yet fully migrated.
 *
 * Displays a centred label so the page is never blank.  Replace this widget
 * with the real page implementation as each phase is completed.
 */
class PlaceholderPage : public BasePage
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a placeholder page with the given label text.
     * @param text Descriptive text shown in the centre of the page.
     * @param parent Parent widget.
     */
    explicit PlaceholderPage(const QString& text, QWidget* parent = nullptr)
        : BasePage(parent)
    {
        auto* layout = new QVBoxLayout(this);
        auto* label  = new QLabel(text, this);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("color: #64748b; font-size: 16px; font-weight: 600;");
        layout->addWidget(label);
    }
};
