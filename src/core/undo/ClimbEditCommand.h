#pragma once

#include <functional>

#include <QSqlDatabase>

#include "UndoCommand.h"
#include "database/ClimbRepository.h"

/// Undo / redo a climb boundary edit (or any field change that can be
/// expressed as a before/after ClimbRecord pair).
class ClimbEditCommand final : public UndoCommand
{
public:
    ClimbEditCommand(const ClimbRecord& before,
                     const ClimbRecord& after,
                     const QString&     dbConnectionName,
                     std::function<void()> refreshCallback);

    void undo() override;
    void redo() override;
    QString description() const override;

private:
    void apply(const ClimbRecord& record);

    ClimbRecord m_before;
    ClimbRecord m_after;
    QString     m_connName;
    std::function<void()> m_refresh;
};
