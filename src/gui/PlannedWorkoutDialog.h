// SPDX-License-Identifier: GPL-3

/**
 * @file PlannedWorkoutDialog.h
 * @brief User interface component for PlannedWorkoutDialog.
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

#include <QDate>
#include <QString>

/**
 * @brief Draft data for a planned workout.
 */
struct PlannedWorkoutDraft
{
    /// @brief Unique identifier (-1 if new).
    int id = -1;

    /// @brief Planned workout date.
    QDate date;

    /// @brief Workout title/name.
    QString title;

    /// @brief Type of workout (e.g., "Recovery", "Threshold").
    QString type;

    /// @brief Planned duration in seconds.
    double durationSeconds = 0.0;

    /// @brief Target Training Stress Score.
    double targetTss = 0.0;

    /// @brief Additional notes or instructions.
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
    /**
     * @brief Constructs planned workout dialog (new workout).
     * @param parent Parent widget.
     */
    explicit PlannedWorkoutDialog(QWidget* parent = nullptr);

    /**
     * @brief Constructs planned workout dialog (edit existing).
     * @param draft Existing workout draft.
     * @param parent Parent widget.
     */
    explicit PlannedWorkoutDialog(const PlannedWorkoutDraft& draft, QWidget* parent = nullptr);

    /**
     * @brief Gets edited workout draft.
     * @return Planned workout data.
     */
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