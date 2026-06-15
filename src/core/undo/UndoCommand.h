// SPDX-License-Identifier: GPL-3

/**
 * @file UndoCommand.h
 * @brief Undo/redo infrastructure for UndoCommand.
 *
 * Defines undo command abstractions and managers used to support reversible edits in the application.
 *
 * Responsibilities:
 * - Provide undo/redo command and management functionality
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QString>

/**
 * @brief Abstract base class for all undo/redo operations.
 *
 * Subclasses implement specific reversible edits by providing undo and redo methods.
 */
class UndoCommand
{
public:
    virtual ~UndoCommand() = default;

    /**
     * @brief Reverses the command (undo operation).
     */
    virtual void undo() = 0;

    /**
     * @brief Re-applies the command (redo operation).
     */
    virtual void redo() = 0;

    /**
     * @brief Returns a human-readable description of the operation.
     * @return Short description shown in menus/status bar.
     */
    virtual QString description() const { return QStringLiteral("Edit"); }
};