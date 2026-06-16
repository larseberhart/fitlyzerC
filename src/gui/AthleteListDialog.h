// SPDX-License-Identifier: GPL-3


#pragma once

#include <QDialog>

#include "database/AthleteRepository.h"

class QTableWidget;
class QPushButton;

/**
 * @brief Dialog for browsing and managing athletes.
 *
 * Lists all athletes, allows selection as current athlete,
 * and provides create/edit/delete operations.
 */
class AthleteListDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AthleteListDialog(AthleteRepository& repo,
                               QWidget* parent = nullptr);

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