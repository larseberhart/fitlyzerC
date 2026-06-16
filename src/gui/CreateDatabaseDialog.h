// SPDX-License-Identifier: GPL-3


#pragma once

#include <QDialog>

class QCheckBox;
class QDialogButtonBox;
class QLineEdit;
class QPushButton;

/**
 * @brief Dialog for creating a new database.
 *
 * Prompts for database name, location, and optionally creates a default athlete.
 */
class CreateDatabaseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateDatabaseDialog(const QString& defaultDirectory, QWidget* parent = nullptr);

    QString databasePath() const;

    bool shouldCreateDefaultAthlete() const;

    QString defaultAthleteFirstName() const;

    QString defaultAthleteLastName() const;

private slots:
    void browseLocation();
    void updateAthleteFields();
    void accept() override;

private:
    QString normalizedDatabaseName() const;

    QLineEdit* m_nameEdit = nullptr;
    QLineEdit* m_locationEdit = nullptr;
    QCheckBox* m_createAthleteCheck = nullptr;
    QLineEdit* m_firstNameEdit = nullptr;
    QLineEdit* m_lastNameEdit = nullptr;
    QPushButton* m_browseButton = nullptr;
    QDialogButtonBox* m_buttonBox = nullptr;
};