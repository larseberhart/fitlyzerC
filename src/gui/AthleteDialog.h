// SPDX-License-Identifier: GPL-3


#pragma once

#include <QDialog>
#include <QList>

#include "database/AthleteRepository.h"

class QLineEdit;
class QDoubleSpinBox;
class QSpinBox;
class QTableWidget;

/**
 * @brief Dialog for creating or editing athlete profile.
 *
 * Allows entry of personal info, FTP history, and weight history.
 */
class AthleteDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief Constructs athlete edit dialog.
     * @param repo Athlete repository.
     * @param athleteId Athlete ID to edit (-1 to create new).
     * @param parent Parent widget.
     */
    explicit AthleteDialog(AthleteRepository& repo,
                           int athleteId = -1,
                           QWidget* parent = nullptr);

    /**
     * @brief Gets ID of saved athlete.
     * @return Athlete ID (valid after Accepted).
     */
    int savedAthleteId() const { return m_savedId; }

private slots:
    void onAccepted();
    void addFtpRow();
    void deleteFtpRow();
    void addWeightRow();
    void deleteWeightRow();

private:
    void loadAthlete();
    void populateFtpTable(const QList<FtpEntry>& entries);
    void populateWeightTable(const QList<WeightEntry>& entries);

    AthleteRepository& m_repo;
    int m_athleteId;
    int m_savedId = -1;

    QLineEdit*      m_firstName  = nullptr;
    QLineEdit*      m_lastName   = nullptr;
    QLineEdit*      m_dob        = nullptr;
    QLineEdit*      m_email      = nullptr;
    QDoubleSpinBox* m_height     = nullptr;
    QSpinBox*       m_ftp        = nullptr;
    QTableWidget*   m_ftpTable   = nullptr;
    QTableWidget*   m_weightTable = nullptr;
};