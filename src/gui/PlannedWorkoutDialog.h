// SPDX-License-Identifier: GPL-3


#pragma once

#include <QDialog>

#include <QDate>
#include <QString>

/**
 * @brief Draft data for a planned workout.
 */
struct PlannedWorkoutDraft
{
    // Unique identifier (-1 if new).
    int id = -1;

    // Planned workout date.
    QDate date;

    // Workout title.
    QString title;

    // Workout type (for example Recovery, Threshold).
    QString type;

    // Planned duration in seconds.
    double durationSeconds = 0.0;

    // Target Training Stress Score.
    double targetTss = 0.0;

    // Additional notes.
    QString notes;
};

class QDateEdit;
class QDoubleSpinBox;
class QLineEdit;
class QTextEdit;

/**
 * @brief Dialog for creating or editing planned workouts.
 */
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