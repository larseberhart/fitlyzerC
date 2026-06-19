// SPDX-License-Identifier: GPL-3

#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

/**
 * @brief Dashboard welcome screen shown when no database is open or no
 *        activities exist.
 *
 * Layout:
 *   - App title and subtitle
 *   - Database row:
 *       Left:  Recently Used
 *       Right: Pinned Databases
 *   - Two-column row:
 *       Left:  Quick Actions (Import / Open / Create / Manage Athletes)
 *       Right: Recent Activities (placeholder — populated in a future update)
 *   - Sync Status section (Garmin Connect / Strava — coming in a future update)
 *
 * The widget is hidden once the user has a database open with activities.
 */
class WelcomeWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the welcome dashboard.
     * @param parent Parent widget.
     */
    explicit WelcomeWidget(QWidget* parent = nullptr);

    /**
     * @brief Switches the subtitle between first-launch and empty-database messages.
     * @param firstLaunch True = first-launch text; false = empty-database text.
     */
    void setFirstLaunch(bool firstLaunch);

signals:
    /// @brief Emitted when the user clicks Import FIT Files.
    void importRequested();

    /// @brief Emitted when the user clicks Open Database.
    void openDatabaseRequested();

    /// @brief Emitted when the user clicks Create Database.
    void createDatabaseRequested();

    /// @brief Emitted when the user clicks Manage Athletes.
    void manageAthletesRequested();

private:
    QLabel*      m_subtitleLabel  = nullptr;
    QPushButton* m_importButton   = nullptr;
    QPushButton* m_openDbButton   = nullptr;
    QPushButton* m_createDbButton = nullptr;
    QPushButton* m_manageAthletesButton = nullptr;
};