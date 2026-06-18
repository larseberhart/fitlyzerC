// SPDX-License-Identifier: GPL-3

#pragma once

#include <QWidget>

/**
 * @brief Abstract base class for all top-level page widgets.
 *
 * Each page corresponds to one entry in NavigationSidebar::Page and occupies
 * one slot in the main window's QStackedWidget (m_pageStack).
 *
 * Subclasses are responsible for building their own UI in the constructor.
 * They may optionally override onActivated() to refresh data when the user
 * navigates to the page.
 */
class BasePage : public QWidget
{
    Q_OBJECT

public:
    explicit BasePage(QWidget* parent = nullptr) : QWidget(parent) {}

    /**
     * @brief Called by MainWindow when this page becomes the active page.
     *
     * Override to refresh stale data or update UI state.  The default
     * implementation does nothing.
     */
    virtual void onActivated() {}
};
