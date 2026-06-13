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

class ActivityBrowser : public QWidget
{
    Q_OBJECT
public:
    explicit ActivityBrowser(DatabaseManager* dbManager, QWidget* parent = nullptr);

    // Reload the table for the given athlete (pass -1 for all athletes).
    void refresh(int athleteId = -1);
    void focusSearch();

signals:
    void activitySelected(int activityId);
    void activityDeleted(int activityId);
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
