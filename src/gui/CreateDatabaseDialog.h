// SPDX-License-Identifier: GPL-3

/**
 * @file CreateDatabaseDialog.h
 * @brief User interface component for CreateDatabaseDialog.
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
    /**
     * @brief Constructs create database dialog.
     * @param defaultDirectory Default location for database.
     * @param parent Parent widget.
     */
    explicit CreateDatabaseDialog(const QString& defaultDirectory, QWidget* parent = nullptr);

    /**
     * @brief Gets the selected database path.
     * @return Full path to new database file.
     */
    QString databasePath() const;

    /**
     * @brief Checks if default athlete should be created.
     * @return True if checkbox is checked.
     */
    bool shouldCreateDefaultAthlete() const;

    /**
     * @brief Gets default athlete first name.
     * @return First name entered.
     */
    QString defaultAthleteFirstName() const;

    /**
     * @brief Gets default athlete last name.
     * @return Last name entered.
     */
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