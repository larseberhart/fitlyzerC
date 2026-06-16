// SPDX-License-Identifier: GPL-3


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