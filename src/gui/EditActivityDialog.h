// SPDX-License-Identifier: GPL-3


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
    explicit EditActivityDialog(const Activity& activity, QWidget* parent = nullptr);

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