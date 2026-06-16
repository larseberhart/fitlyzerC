// SPDX-License-Identifier: GPL-3


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
    explicit EditNotesDialog(const QString& notes,
                             const QString& weatherNotes,
                             QWidget* parent = nullptr);

    QString notes()        const;

    QString weatherNotes() const;

private:
    QTextEdit* m_notes   = nullptr;
    QLineEdit* m_weather = nullptr;
};