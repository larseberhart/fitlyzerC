// SPDX-License-Identifier: GPL-3

/**
 * @file UndoManager.h
 * @brief Undo/redo infrastructure for UndoManager.
 *
 * Defines undo command abstractions and managers used to support reversible edits in the application.
 *
 * Responsibilities:
 * - Provide undo/redo command and management functionality
 *
 * @author Lars EBERHART
 */

#pragma once

#include <deque>
#include <memory>

#include <QObject>
#include <QString>

#include "UndoCommand.h"

/**
 * @brief Central undo/redo stack manager.
 *
 * Manages a queue of reversible commands, supporting undo and redo operations
 * with change notifications for UI updates.
 */
class UndoManager : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Constructs the undo manager.
     * @param maxStackSize Maximum number of commands to retain (default 50).
     * @param parent Qt parent object.
     */
    explicit UndoManager(int maxStackSize = 50, QObject* parent = nullptr);

    /**
     * @brief Pushes a newly executed command onto the undo stack.
     *
     * Clears the redo stack when a new command is pushed.
     * @param cmd Unique pointer to the command to execute.
     */
    void push(std::unique_ptr<UndoCommand> cmd);

    /**
     * @brief Clears all undo and redo history.
     */
    void clear();

    /**
     * @brief Checks if undo is available.
     * @return True if there are commands to undo.
     */
    bool    canUndo() const;

    /**
     * @brief Checks if redo is available.
     * @return True if there are commands to redo.
     */
    bool    canRedo() const;

    /**
     * @brief Gets description of the next undo operation.
     * @return Human-readable undo description.
     */
    QString undoDescription() const;

    /**
     * @brief Gets description of the next redo operation.
     * @return Human-readable redo description.
     */
    QString redoDescription() const;

public slots:
    /**
     * @brief Performs undo operation on the top command.
     */
    void undo();

    /**
     * @brief Performs redo operation on the next command.
     */
    void redo();

signals:
    /**
     * @brief Emitted when undo availability changes.
     */
    void canUndoChanged(bool canUndo);

    /**
     * @brief Emitted when redo availability changes.
     */
    void canRedoChanged(bool canRedo);

    /**
     * @brief Emitted when undo description changes.
     */
    void undoDescriptionChanged(const QString& desc);

    /**
     * @brief Emitted when redo description changes.
     */
    void redoDescriptionChanged(const QString& desc);

private:
    void notify();

    int m_maxStackSize;
    std::deque<std::unique_ptr<UndoCommand>> m_undoStack;
    std::deque<std::unique_ptr<UndoCommand>> m_redoStack;
};