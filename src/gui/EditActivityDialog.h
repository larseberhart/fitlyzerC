// SPDX-License-Identifier: GPL-3

/**
 * @file EditActivityDialog.h
 * @brief User interface component for EditActivityDialog.
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

#include "database/ActivityRepository.h"

class QDoubleSpinBox;
class QLineEdit;
class QTextEdit;
class QSpinBox;

/**
 * @brief Dialog for editing activity metadata.
 *
 * Allows editing of weather, temperature, RPE, fatigue, sleep, weight,
 * bike, equipment, and notes.
 */
class EditActivityDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructs activity edit dialog.
     * @param activity Activity to edit.
     * @param parent Parent widget.
     */
    explicit EditActivityDialog(const Activity& activity, QWidget* parent = nullptr);

    /**
     * @brief Gets updated activity with edited values.
     * @return Modified activity record.
     */
    Activity updatedActivity() const;

private:
    Activity m_initial;

    QDoubleSpinBox* m_temperatureSpin = nullptr;
    QLineEdit* m_weatherEdit = nullptr;
    QLineEdit* m_windEdit = nullptr;
    QSpinBox* m_rpeSpin = nullptr;
    QSpinBox* m_fatigueSpin = nullptr;
    QDoubleSpinBox* m_sleepSpin = nullptr;
    QDoubleSpinBox* m_weightSpin = nullptr;
    QLineEdit* m_bikeEdit = nullptr;
    QLineEdit* m_equipmentEdit = nullptr;
    QTextEdit* m_notesEdit = nullptr;
};