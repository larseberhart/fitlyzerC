#include "IntervalEditorDialog.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>

IntervalEditorDialog::IntervalEditorDialog(double startSeconds,
                                           double endSeconds,
                                           QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Create Interval");
    setMinimumWidth(420);

    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout;

    m_nameEdit = new QLineEdit("Manual Interval", this);
    m_typeEdit = new QLineEdit("Manual", this);

    m_startSpin = new QDoubleSpinBox(this);
    m_startSpin->setRange(0.0, 86400.0);
    m_startSpin->setSuffix(" s");
    m_startSpin->setValue(startSeconds);

    m_endSpin = new QDoubleSpinBox(this);
    m_endSpin->setRange(0.0, 86400.0);
    m_endSpin->setSuffix(" s");
    m_endSpin->setValue(endSeconds);

    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setMinimumHeight(100);

    form->addRow("Name:", m_nameEdit);
    form->addRow("Type:", m_typeEdit);
    form->addRow("Start:", m_startSpin);
    form->addRow("End:", m_endSpin);
    form->addRow("Notes:", m_notesEdit);

    layout->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

QString IntervalEditorDialog::intervalName() const
{
    return m_nameEdit ? m_nameEdit->text().trimmed() : QString();
}

QString IntervalEditorDialog::intervalType() const
{
    return m_typeEdit ? m_typeEdit->text().trimmed() : QString();
}

QString IntervalEditorDialog::intervalNotes() const
{
    return m_notesEdit ? m_notesEdit->toPlainText().trimmed() : QString();
}
