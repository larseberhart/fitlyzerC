// SPDX-License-Identifier: GPL-3

/**
 * @file ActivityBrowser.h
 * @brief User interface component for ActivityBrowser.
 *
 * Defines dialogs, widgets, controllers, and UI workflows used by the FitlyzerC desktop application.
 *
 * Responsibilities:
 * - Provide interactive user interface behavior and presentation
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QWidget>

#include "database/DatabaseManager.h"
#include "database/ActivityRepository.h"

class QTableWidget;
class QComboBox;
class QLineEdit;
class QLabel;

/**
 * @brief Browse and manage activities.
 *
 * Displays activities in a filterable table, supports drag-drop import,
 * and provides context menu for activity management.
 */
class ActivityBrowser : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief Constructs activity browser.
     * @param dbManager Database manager.
     * @param parent Parent widget.
     */
    explicit ActivityBrowser(DatabaseManager* dbManager, QWidget* parent = nullptr);

    /**
     * @brief Refreshes activity list.
     * @param athleteId Athlete to show (-1 for all).
     */
    void refresh(int athleteId = -1);

    /**
     * @brief Focuses search field.
     */
    void focusSearch();

signals:
    /// \signal Emitted when activity is selected in table.
    /// \param activityId Selected activity ID.
    void activitySelected(int activityId);

    /// \signal Emitted when activity is deleted.
    /// \param activityId Deleted activity ID.
    void activityDeleted(int activityId);

    /// \signal Emitted when FIT files are dropped onto widget.
    /// \param filePaths List of dropped file paths.
    void fitFilesDropped(const QStringList& filePaths);

protected:
    /// @brief Handles drag-enter for file drop validation.
    void dragEnterEvent(QDragEnterEvent* event) override;

    /// @brief Handles drop event for FIT file import.
    void dropEvent(QDropEvent* event) override;

private:
    void applyFilters();
    void showContextMenu(const QPoint& pos);
    void editNotes(int activityId);
    void deleteActivity(int activityId);

    DatabaseManager* m_dbManager = nullptr;
    QLabel*          m_emptyStateLabel = nullptr;
    QLineEdit*       m_searchEdit = nullptr;
    QComboBox*       m_rangeFilter = nullptr;
    QTableWidget*    m_table     = nullptr;
    int              m_athleteId = -1;
};