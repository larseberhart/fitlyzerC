// SPDX-License-Identifier: GPL-3

#include "FitnessPage.h"

#include <QVBoxLayout>

#include "../../charts/FitnessChartWidget.h"

/**
 * @brief Constructs the Fitness page.
 * @param fitnessChart FitnessChartWidget to embed in this page.
 * @param parent       Parent widget.
 */
FitnessPage::FitnessPage(FitnessChartWidget* fitnessChart, QWidget* parent)
    : BasePage(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    if (fitnessChart)
        layout->addWidget(fitnessChart);
}
