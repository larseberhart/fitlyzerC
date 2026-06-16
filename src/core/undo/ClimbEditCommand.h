// SPDX-License-Identifier: GPL-3


#pragma once

#include <functional>

#include <QSqlDatabase>

#include "UndoCommand.h"
#include "database/ClimbRepository.h"

/**
 * @brief Undo/redo command for climb boundary or field edits.
 *
 * Stores before/after ClimbRecord pairs and applies changes through
 * the ClimbRepository during undo/redo operations.
 */
class ClimbEditCommand final : public UndoCommand
{
public:
    /**
     * @brief Constructs climb edit command.
     * @param before Previous climb record state.
     * @param after New climb record state.
     * @param dbConnectionName Database connection name.
     * @param refreshCallback Function to call after applying changes.
     */
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