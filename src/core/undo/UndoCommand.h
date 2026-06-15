#pragma once

#include <QString>

/// Abstract base for all undo/redo operations.
class UndoCommand
{
public:
    virtual ~UndoCommand() = default;

    virtual void undo() = 0;
    virtual void redo() = 0;

    /// Short human-readable label shown in menus / status bar.
    virtual QString description() const { return QStringLiteral("Edit"); }
};
