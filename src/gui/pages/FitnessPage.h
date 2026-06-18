// SPDX-License-Identifier: GPL-3

#pragma once

#include "BasePage.h"

class FitnessChartWidget;

/**
 * @brief Top-level page for fitness tracking.
 *
 * Displays CTL (Chronic Training Load), ATL (Acute Training Load),
 * TSB (Training Stress Balance), and FTP history.
 *
 * Future additions: Readiness score, fatigue/recovery estimates,
 * training recommendations.
 */
class FitnessPage : public BasePage
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the Fitness page.
     * @param fitnessChart The FitnessChartWidget (ownership transfers here).
     * @param parent       Parent widget.
     */
    explicit FitnessPage(FitnessChartWidget* fitnessChart, QWidget* parent = nullptr);
};
