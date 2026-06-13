#pragma once

#include <QDialog>

#include <QDate>
#include <QString>

struct PlannedWorkoutDraft
{
    int id = -1;
    QDate date;
    QString title;
    QString type;
    double durationSeconds = 0.0;
    double targetTss = 0.0;
    QString notes;
};

class QDateEdit;
class QDoubleSpinBox;
class QLineEdit;
class QTextEdit;

class PlannedWorkoutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PlannedWorkoutDialog(QWidget* parent = nullptr);
    explicit PlannedWorkoutDialog(const PlannedWorkoutDraft& draft, QWidget* parent = nullptr);

    PlannedWorkoutDraft draft() const;

private:
    QDateEdit* m_dateEdit = nullptr;
    QLineEdit* m_titleEdit = nullptr;
    QLineEdit* m_typeEdit = nullptr;
    QDoubleSpinBox* m_durationSpin = nullptr;
    QDoubleSpinBox* m_tssSpin = nullptr;
    QTextEdit* m_notesEdit = nullptr;
    int m_id = -1;
};
