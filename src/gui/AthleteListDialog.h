#pragma once

#include <QDialog>

#include "database/AthleteRepository.h"

class QTableWidget;
class QPushButton;

// Lists all athletes and lets the user select one as the current athlete,
// or create/edit/delete athletes.
class AthleteListDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AthleteListDialog(AthleteRepository& repo,
                               QWidget* parent = nullptr);

    // The id of the selected athlete after Accepted (or -1).
    int selectedAthleteId() const { return m_selectedId; }

private slots:
    void refresh();
    void newAthlete();
    void editAthlete();
    void deleteAthlete();
    void selectAthlete();

private:
    AthleteRepository& m_repo;
    int m_selectedId = -1;

    QTableWidget* m_table      = nullptr;
    QPushButton*  m_editBtn    = nullptr;
    QPushButton*  m_deleteBtn  = nullptr;
    QPushButton*  m_selectBtn  = nullptr;
};
