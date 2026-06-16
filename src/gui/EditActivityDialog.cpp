// SPDX-License-Identifier: GPL-3


#include "EditActivityDialog.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>

EditActivityDialog::EditActivityDialog(const Activity& activity, QWidget* parent)
    : QDialog(parent)
    , m_initial(activity)
{
    setWindowTitle("Activity Properties");
    setMinimumWidth(460);

    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout;

    m_temperatureSpin = new QDoubleSpinBox(this);
    m_temperatureSpin->setRange(-40.0, 60.0);
    m_temperatureSpin->setDecimals(1);
    m_temperatureSpin->setSuffix(" C");
    m_temperatureSpin->setValue(activity.temperature);

    m_weatherEdit = new QLineEdit(activity.weather, this);
    m_windEdit = new QLineEdit(activity.wind, this);

    m_rpeSpin = new QSpinBox(this);
    m_rpeSpin->setRange(0, 10);
    m_rpeSpin->setValue(activity.rpe);

    m_fatigueSpin = new QSpinBox(this);
    m_fatigueSpin->setRange(0, 10);
    m_fatigueSpin->setValue(activity.fatigue);

    m_sleepSpin = new QDoubleSpinBox(this);
    m_sleepSpin->setRange(0.0, 24.0);
    m_sleepSpin->setDecimals(1);
    m_sleepSpin->setSuffix(" h");
    m_sleepSpin->setValue(activity.sleepHours);

    m_weightSpin = new QDoubleSpinBox(this);
    m_weightSpin->setRange(0.0, 200.0);
    m_weightSpin->setDecimals(1);
    m_weightSpin->setSuffix(" kg");
    m_weightSpin->setValue(activity.weightKg);

    m_bikeEdit = new QLineEdit(activity.bike, this);
    m_equipmentEdit = new QLineEdit(activity.equipment, this);

    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setMinimumHeight(120);
    m_notesEdit->setPlainText(activity.notes);

    form->addRow("Temperature:", m_temperatureSpin);
    form->addRow("Weather:", m_weatherEdit);
    form->addRow("Wind:", m_windEdit);
    form->addRow("RPE:", m_rpeSpin);
    form->addRow("Fatigue:", m_fatigueSpin);
    form->addRow("Sleep:", m_sleepSpin);
    form->addRow("Weight:", m_weightSpin);
    form->addRow("Bike:", m_bikeEdit);
    form->addRow("Equipment:", m_equipmentEdit);
    form->addRow("Notes:", m_notesEdit);

    layout->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

Activity EditActivityDialog::updatedActivity() const
{
    Activity out = m_initial;
    out.temperature = m_temperatureSpin ? m_temperatureSpin->value() : m_initial.temperature;
    out.weather = m_weatherEdit ? m_weatherEdit->text().trimmed() : m_initial.weather;
    out.wind = m_windEdit ? m_windEdit->text().trimmed() : m_initial.wind;
    out.rpe = m_rpeSpin ? m_rpeSpin->value() : m_initial.rpe;
    out.fatigue = m_fatigueSpin ? m_fatigueSpin->value() : m_initial.fatigue;
    out.sleepHours = m_sleepSpin ? m_sleepSpin->value() : m_initial.sleepHours;
    out.weightKg = m_weightSpin ? m_weightSpin->value() : m_initial.weightKg;
    out.bike = m_bikeEdit ? m_bikeEdit->text().trimmed() : m_initial.bike;
    out.equipment = m_equipmentEdit ? m_equipmentEdit->text().trimmed() : m_initial.equipment;
    out.notes = m_notesEdit ? m_notesEdit->toPlainText().trimmed() : m_initial.notes;
    return out;
}