// SPDX-License-Identifier: GPL-3

/**
 * @file AboutDialog.h
 * @brief User interface component for AboutDialog.
 *
 * Defines dialogs, widgets, controllers, and UI workflows used by the FitlyzerC desktop application.
 *
 * Responsibilities:
 * - Provide interactive user interface behavior and presentation
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QDialog>

/**
 * @brief Displays application about/splash dialog.
 */
class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructs about dialog.
     * @param parent Parent widget.
     */
    explicit AboutDialog(QWidget* parent = nullptr);
};