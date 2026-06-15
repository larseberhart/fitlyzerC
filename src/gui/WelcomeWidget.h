// SPDX-License-Identifier: GPL-3

/**
 * @file WelcomeWidget.h
 * @brief User interface component for WelcomeWidget.
 *
 * Defines dialogs, widgets, controllers, and UI workflows used by the FitlyzerC desktop application.
 *
 * Responsibilities:
 * - Provide interactive user interface behavior and presentation
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

/**
 * @brief Welcome screen for first-time users.
 *
 * Provides quick action buttons for creating database, opening existing,
 * or importing activities.
 */
class WelcomeWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructs welcome widget.
     * @param parent Parent widget.
     */
    explicit WelcomeWidget(QWidget* parent = nullptr);

    /**
     * @brief Sets first launch mode.
     * @param firstLaunch True to show first-launch UI.
     */
    void setFirstLaunch(bool firstLaunch);

signals:
    /// \signal Emitted when user requests activity import.
    void importRequested();

    /// \signal Emitted when user requests to open existing database.
    void openDatabaseRequested();

    /// \signal Emitted when user requests to create new database.
    void createDatabaseRequested();

private:
    QLabel* m_subtitleLabel = nullptr;
    QPushButton* m_importButton = nullptr;
    QPushButton* m_openDbButton = nullptr;
    QPushButton* m_createDbButton = nullptr;
};