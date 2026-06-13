#pragma once

#include <QDialog>
#include <QList>

#include "database/AthleteRepository.h"

class QLineEdit;
class QDoubleSpinBox;
class QSpinBox;
class QTableWidget;

// Dialog for creating or editing a single athlete,
// including FTP history and weight history sub-tables.
class AthleteDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AthleteDialog(AthleteRepository& repo,
                           int athleteId = -1,   // -1 = create new
                           QWidget* parent = nullptr);

    // Returns the id of the saved athlete (valid after Accepted).
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
