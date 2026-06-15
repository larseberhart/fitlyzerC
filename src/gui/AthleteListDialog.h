// SPDX-License-Identifier: GPL-3

/**
 * @file AthleteListDialog.h
 * @brief User interface component for AthleteListDialog.
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
    /**
     * @brief Constructs athlete list dialog.
     * @param repo Athlete repository.
     * @param parent Parent widget.
     */
    explicit AthleteListDialog(AthleteRepository& repo,
                               QWidget* parent = nullptr);

    /**
     * @brief Gets the selected athlete ID.
     * @return Athlete ID or -1 if none selected.
     */
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