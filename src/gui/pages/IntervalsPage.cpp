// SPDX-License-Identifier: GPL-3

#include "IntervalsPage.h"

#include <QLabel>
#include <QVBoxLayout>

/**
 * @brief Constructs the Intervals page.
 *
 * If intervalsContent is provided, embeds it in a full-height layout.
 * Otherwise shows a placeholder label directing the user to the Charts page
 * where interval selection and chart-context navigation are available.
 *
 * @param intervalsContent Assembled intervals widget, or nullptr for placeholder.
 * @param parent           Parent widget.
 */
IntervalsPage::IntervalsPage(QWidget* intervalsContent, QWidget* parent)
    : BasePage(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    if (intervalsContent)
    {
        layout->addWidget(intervalsContent);
    }
    else
    {
        // Placeholder: intervals are accessible from the Charts page.
        auto* label = new QLabel(
            "Interval analysis is available on the Charts page.\n"
            "Select an interval there to zoom the chart to that effort.",
            this);
        label->setAlignment(Qt::AlignCenter);
        label->setWordWrap(true);
        label->setStyleSheet("color: #64748b; font-size: 15px;");
        layout->addWidget(label);
    }
}
