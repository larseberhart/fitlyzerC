#include "PlannedWorkoutDialog.h"
#include "core/settings/DateFormatter.h"

#include <QDate>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>

PlannedWorkoutDialog::PlannedWorkoutDialog(QWidget* parent)
    : PlannedWorkoutDialog(PlannedWorkoutDraft{}, parent)
{}

PlannedWorkoutDialog::PlannedWorkoutDialog(const PlannedWorkoutDraft& draft, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Planned Workout");
    setMinimumWidth(420);

    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout;

    m_dateEdit = new QDateEdit(this);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat(DateFormatter::qtDatePattern());
    m_dateEdit->setDate(draft.date.isValid() ? draft.date : QDate::currentDate());

    m_titleEdit = new QLineEdit(draft.title, this);
    m_typeEdit = new QLineEdit(draft.type, this);

    m_durationSpin = new QDoubleSpinBox(this);
    m_durationSpin->setRange(0.0, 12.0);
    m_durationSpin->setDecimals(2);
    m_durationSpin->setSuffix(" h");
    m_durationSpin->setValue(draft.durationSeconds / 3600.0);

    m_tssSpin = new QDoubleSpinBox(this);
    m_tssSpin->setRange(0.0, 1000.0);
    m_tssSpin->setDecimals(1);
    m_tssSpin->setValue(draft.targetTss);

    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setMinimumHeight(90);
    m_notesEdit->setPlainText(draft.notes);

    m_id = draft.id;

    form->addRow("Date:", m_dateEdit);
    form->addRow("Title:", m_titleEdit);
    form->addRow("Type:", m_typeEdit);
    form->addRow("Duration:", m_durationSpin);
    form->addRow("Target TSS:", m_tssSpin);
    form->addRow("Notes:", m_notesEdit);

    layout->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

PlannedWorkoutDraft PlannedWorkoutDialog::draft() const
{
    PlannedWorkoutDraft out;
    out.id = m_id;
    out.date = m_dateEdit ? m_dateEdit->date() : QDate();
    out.title = m_titleEdit ? m_titleEdit->text().trimmed() : QString();
    out.type = m_typeEdit ? m_typeEdit->text().trimmed() : QString();
    out.durationSeconds = (m_durationSpin ? m_durationSpin->value() : 0.0) * 3600.0;
    out.targetTss = m_tssSpin ? m_tssSpin->value() : 0.0;
    out.notes = m_notesEdit ? m_notesEdit->toPlainText().trimmed() : QString();
    return out;
}
