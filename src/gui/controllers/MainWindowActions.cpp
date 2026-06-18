// SPDX-License-Identifier: GPL-3

#include "MainWindowActions.h"

#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QComboBox>
#include <QLabel>
#include <QStyle>

/**
 * @brief Constructs the actions manager.
 * @param mainWindow Pointer to the main window.
 * @param parent Parent object.
 */
MainWindowActions::MainWindowActions(QMainWindow* mainWindow, QObject* parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
{
}

/**
 * @brief Destructor.
 */
MainWindowActions::~MainWindowActions() = default;

/**
 * @brief Creates and sets up all menus.
 */
void MainWindowActions::buildMenus()
{
    if (!m_mainWindow)
        return;

    createFileMenu();
    createEditMenu();
    createViewMenu();
    createAnalysisMenu();
    createToolsMenu();
    createHelpMenu();
}

/**
 * @brief Creates and sets up the main toolbar.
 */
void MainWindowActions::buildToolbar()
{
    if (!m_mainWindow)
        return;

    // TODO: Extract toolbar creation from MainWindow
}

/**
 * @brief Sets up keyboard shortcuts.
 */
void MainWindowActions::setupShortcuts()
{
    // TODO: Extract keyboard shortcuts setup from MainWindow
}

/**
 * @brief Helper to create File menu.
 */
void MainWindowActions::createFileMenu()
{
    if (!m_mainWindow)
        return;

    QMenu* fileMenu = m_mainWindow->menuBar()->addMenu("&File");

    // TODO: Extract File menu creation from MainWindow
}

/**
 * @brief Helper to create Edit menu.
 */
void MainWindowActions::createEditMenu()
{
    if (!m_mainWindow)
        return;

    QMenu* editMenu = m_mainWindow->menuBar()->addMenu("&Edit");

    // TODO: Extract Edit menu creation from MainWindow
}

/**
 * @brief Helper to create View menu.
 */
void MainWindowActions::createViewMenu()
{
    if (!m_mainWindow)
        return;

    QMenu* viewMenu = m_mainWindow->menuBar()->addMenu("&View");

    // TODO: Extract View menu creation from MainWindow
}

/**
 * @brief Helper to create Analysis menu.
 */
void MainWindowActions::createAnalysisMenu()
{
    if (!m_mainWindow)
        return;

    QMenu* analysisMenu = m_mainWindow->menuBar()->addMenu("&Analysis");

    // TODO: Extract Analysis menu creation from MainWindow
}

/**
 * @brief Helper to create Tools menu.
 */
void MainWindowActions::createToolsMenu()
{
    if (!m_mainWindow)
        return;

    QMenu* toolsMenu = m_mainWindow->menuBar()->addMenu("&Tools");

    // TODO: Extract Tools menu creation from MainWindow
}

/**
 * @brief Helper to create Help menu.
 */
void MainWindowActions::createHelpMenu()
{
    if (!m_mainWindow)
        return;

    QMenu* helpMenu = m_mainWindow->menuBar()->addMenu("&Help");

    // TODO: Extract Help menu creation from MainWindow
}
