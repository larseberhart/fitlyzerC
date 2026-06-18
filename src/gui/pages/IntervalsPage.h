// SPDX-License-Identifier: GPL-3

#pragma once

#include "BasePage.h"

class QTableWidget;
class QLabel;
class QCheckBox;
class QStackedLayout;

/**
 * @brief Top-level page for interval analysis and management.
 *
 * Displays the interval table (auto-detected and manually saved intervals)
 * with summary statistics for the selected interval.
 *
 * Toolbar actions: Add, Edit, Delete.
 *
 * Future additions: Interval Comparison, Interval Library, Structured
 * Workout Builder.
 */
class IntervalsPage : public BasePage
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the Intervals page.
     * @param intervalsContent  Pre-assembled intervals panel (table + toolbar + summary).
     * @param parent            Parent widget.
     */
    explicit IntervalsPage(QWidget* intervalsContent, QWidget* parent = nullptr);
};
