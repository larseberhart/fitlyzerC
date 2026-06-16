// SPDX-License-Identifier: GPL-3


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
    explicit ActivityBrowser(DatabaseManager* dbManager, QWidget* parent = nullptr);

    void refresh(int athleteId = -1);

    void focusSearch();

signals:
    // Emitted when an activity is selected in the table.
    void activitySelected(int activityId);

    // Emitted when an activity is deleted.
    void activityDeleted(int activityId);

    // Emitted when FIT files are dropped onto the widget.
    void fitFilesDropped(const QStringList& filePaths);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;

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