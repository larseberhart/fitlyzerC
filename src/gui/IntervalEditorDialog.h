#pragma once

#include <QDialog>
#include <QString>

class QDoubleSpinBox;
class QLineEdit;
class QTextEdit;

class IntervalEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IntervalEditorDialog(double startSeconds,
                                  double endSeconds,
                                  QWidget* parent = nullptr);

    QString intervalName() const;
    QString intervalType() const;
    QString intervalNotes() const;

private:
    QLineEdit* m_nameEdit = nullptr;
    QLineEdit* m_typeEdit = nullptr;
    QTextEdit* m_notesEdit = nullptr;
    QDoubleSpinBox* m_startSpin = nullptr;
    QDoubleSpinBox* m_endSpin = nullptr;
};
