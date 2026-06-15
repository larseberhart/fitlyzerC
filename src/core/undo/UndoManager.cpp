// SPDX-License-Identifier: GPL-3

/**
 * @file UndoManager.cpp
 * @brief Undo/redo infrastructure for UndoManager.
 *
 * Defines undo command abstractions and managers used to support reversible edits in the application.
 *
 * Responsibilities:
 * - Provide undo/redo command and management functionality
 *
 * @author Lars EBERHART
 */

#include "UndoManager.h"

UndoManager::UndoManager(int maxStackSize, QObject* parent)
    : QObject(parent)
    , m_maxStackSize(maxStackSize)
{}

void UndoManager::push(std::unique_ptr<UndoCommand> cmd)
{
    m_redoStack.clear();
    m_undoStack.push_back(std::move(cmd));

    // Trim oldest entries when the stack exceeds the limit.
    // std::deque::pop_front() is O(1), whereas vector::erase(begin()) is O(n).
    while (static_cast<int>(m_undoStack.size()) > m_maxStackSize)
        m_undoStack.pop_front();

    notify();
}

void UndoManager::clear()
{
    m_undoStack.clear();
    m_redoStack.clear();
    notify();
}

bool UndoManager::canUndo() const { return !m_undoStack.empty(); }
bool UndoManager::canRedo() const { return !m_redoStack.empty(); }

QString UndoManager::undoDescription() const
{
    return m_undoStack.empty() ? QString() : m_undoStack.back()->description();
}

QString UndoManager::redoDescription() const
{
    return m_redoStack.empty() ? QString() : m_redoStack.back()->description();
}

void UndoManager::undo()
{
    if (m_undoStack.empty())
        return;

    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    cmd->undo();
    m_redoStack.push_back(std::move(cmd));
    notify();
}

void UndoManager::redo()
{
    if (m_redoStack.empty())
        return;

    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    cmd->redo();
    m_undoStack.push_back(std::move(cmd));
    notify();
}

void UndoManager::notify()
{
    emit canUndoChanged(canUndo());
    emit canRedoChanged(canRedo());
    emit undoDescriptionChanged(undoDescription());
    emit redoDescriptionChanged(redoDescription());
}