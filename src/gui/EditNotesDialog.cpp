// SPDX-License-Identifier: GPL-3

/**
 * @file EditNotesDialog.cpp
 * @brief User interface component for EditNotesDialog.
 *
 * Defines dialogs, widgets, controllers, and UI workflows used by the FitlyzerC desktop application.
 *
 * Responsibilities:
 * - Provide interactive user interface behavior and presentation
 *
 * @author Lars EBERHART
 */

#include "EditNotesDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>

EditNotesDialog::EditNotesDialog(const QString& notes,
                                 const QString& weatherNotes,
                                 QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Edit Activity Notes");
    setMinimumWidth(420);

    auto* layout = new QVBoxLayout(this);

    auto* form = new QFormLayout;
    m_notes = new QTextEdit;
    m_notes->setPlainText(notes);
    m_notes->setMinimumHeight(120);
    form->addRow("Notes:", m_notes);

    m_weather = new QLineEdit(weatherNotes);
    form->addRow("Weather:", m_weather);

    layout->addLayout(form);

    auto* bbox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(bbox);

    connect(bbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bbox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString EditNotesDialog::notes() const
{
    return m_notes->toPlainText();
}

QString EditNotesDialog::weatherNotes() const
{
    return m_weather->text();
}