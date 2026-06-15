// SPDX-License-Identifier: GPL-3

/**
 * @file IntervalEditorDialog.h
 * @brief User interface component for IntervalEditorDialog.
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
#include <QString>

class QDoubleSpinBox;
class QLineEdit;
class QTextEdit;

/**
 * @brief Dialog for editing interval metadata.
 *
 * Allows entry of interval name, type, and notes.
 */
class IntervalEditorDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructs interval editor dialog.
     * @param startSeconds Start time in seconds.
     * @param endSeconds End time in seconds.
     * @param parent Parent widget.
     */
    explicit IntervalEditorDialog(double startSeconds,
                                  double endSeconds,
                                  QWidget* parent = nullptr);

    /**
     * @brief Gets entered interval name.
     * @return Interval name.
     */
    QString intervalName() const;

    /**
     * @brief Gets entered interval type.
     * @return Interval type.
     */
    QString intervalType() const;

    /**
     * @brief Gets entered interval notes.
     * @return Notes text.
     */
    QString intervalNotes() const;

private:
    QLineEdit* m_nameEdit = nullptr;
    QLineEdit* m_typeEdit = nullptr;
    QTextEdit* m_notesEdit = nullptr;
    QDoubleSpinBox* m_startSpin = nullptr;
    QDoubleSpinBox* m_endSpin = nullptr;
};