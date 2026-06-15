#pragma once

#include <memory>
#include <vector>

#include <QObject>
#include <QString>

#include "UndoCommand.h"

/// Central undo / redo stack.  Owns all commands.
class UndoManager : public QObject
{
    Q_OBJECT
public:
    explicit UndoManager(int maxStackSize = 50, QObject* parent = nullptr);

    /// Push a newly executed command.  Clears the redo stack.
    void push(std::unique_ptr<UndoCommand> cmd);

    /// Remove all commands.
    void clear();

    bool    canUndo() const;
    bool    canRedo() const;
    QString undoDescription() const;
    QString redoDescription() const;

public slots:
    void undo();
    void redo();

signals:
    void canUndoChanged(bool canUndo);
    void canRedoChanged(bool canRedo);
    void undoDescriptionChanged(const QString& desc);
    void redoDescriptionChanged(const QString& desc);

private:
    void notify();

    int m_maxStackSize;
    std::vector<std::unique_ptr<UndoCommand>> m_undoStack;
    std::vector<std::unique_ptr<UndoCommand>> m_redoStack;
};
