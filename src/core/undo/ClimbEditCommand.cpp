// SPDX-License-Identifier: GPL-3


#include "ClimbEditCommand.h"

#include <QSqlDatabase>

ClimbEditCommand::ClimbEditCommand(const ClimbRecord& before,
                                   const ClimbRecord& after,
                                   const QString&     dbConnectionName,
                                   std::function<void()> refreshCallback)
    : m_before(before)
    , m_after(after)
    , m_connName(dbConnectionName)
    , m_refresh(std::move(refreshCallback))
{}

void ClimbEditCommand::undo()
{
    apply(m_before);
}

void ClimbEditCommand::redo()
{
    apply(m_after);
}

QString ClimbEditCommand::description() const
{
    return QStringLiteral("Edit Climb Boundary");
}

void ClimbEditCommand::apply(const ClimbRecord& record)
{
    auto db = QSqlDatabase::database(m_connName);
    if (db.isValid() && db.isOpen())
    {
        ClimbRepository repo(db);
        repo.updateClimb(record);
    }
    if (m_refresh)
        m_refresh();
}