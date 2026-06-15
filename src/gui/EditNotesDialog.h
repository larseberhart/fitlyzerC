// SPDX-License-Identifier: GPL-3

/**
 * @file EditNotesDialog.h
 * @brief User interface component for EditNotesDialog.
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

class QLineEdit;
class QTextEdit;

/**
 * @brief Dialog for editing activity notes and weather.
 */
class EditNotesDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief Constructs notes editor dialog.
     * @param notes Initial activity notes.
     * @param weatherNotes Initial weather notes.
     * @param parent Parent widget.
     */
    explicit EditNotesDialog(const QString& notes,
                             const QString& weatherNotes,
                             QWidget* parent = nullptr);

    /**
     * @brief Gets edited activity notes.
     * @return Notes text.
     */
    QString notes()        const;

    /**
     * @brief Gets edited weather notes.
     * @return Weather text.
     */
    QString weatherNotes() const;

private:
    QTextEdit* m_notes   = nullptr;
    QLineEdit* m_weather = nullptr;
};