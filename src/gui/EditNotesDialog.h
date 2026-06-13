#pragma once

#include <QDialog>
#include <QString>

class QLineEdit;
class QTextEdit;

// Simple dialog for editing an activity's notes and weather notes.
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
