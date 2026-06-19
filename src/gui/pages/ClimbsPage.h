// SPDX-License-Identifier: GPL-3

#pragma once

#include "BasePage.h"

class QWidget;
class QToolBar;

/**
 * @brief Top-level page for climb detection and analysis.
 *
 * Hosts the climb altitude chart (with boundary-editing enabled),
 * the climbs table, and the per-climb statistics summary.
 *
 * Toolbar actions: Add, Edit, Merge, Delete, Undo, Redo.
 */
class ClimbsPage : public BasePage
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the Climbs page.
     * @param contentWidget    Pre-assembled climb charts + table widget.
     * @param parent           Parent widget.
     */
    explicit ClimbsPage(QWidget* contentWidget,
                        QWidget* parent = nullptr);

signals:
    void addRequested();
    void editRequested();
    void mergeRequested();
    void deleteRequested();
    void undoRequested();
    void redoRequested();

private:
    QToolBar* m_toolbar = nullptr;
};
