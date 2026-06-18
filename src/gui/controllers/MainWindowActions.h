// SPDX-License-Identifier: GPL-3

#pragma once

#include <QObject>
#include <QAction>
#include <memory>

class QMainWindow;
class QMenu;
class QToolBar;
class QComboBox;
class QPushButton;

/**
 * @brief Manages menus, toolbars, actions, and keyboard shortcuts.
 *
 * Handles:
 * - File menu (import, open database, create database)
 * - Edit menu (athlete management, settings)
 * - View menu (full screen)
 * - Analysis menu (detect climbs, detect intervals)
 * - Tools menu (create video, athlete management)
 * - Help menu (about)
 * - Main toolbar (import, search, athlete selector, import status)
 * - Keyboard shortcuts (F11, Ctrl+0, Ctrl+Z, etc.)
 */
class MainWindowActions : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the actions manager.
     * @param mainWindow Pointer to the main window.
     * @param parent Parent object.
     */
    explicit MainWindowActions(QMainWindow* mainWindow, QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~MainWindowActions() override;

    /**
     * @brief Creates and sets up all menus.
     */
    void buildMenus();

    /**
     * @brief Creates and sets up the main toolbar.
     */
    void buildToolbar();

    /**
     * @brief Sets up keyboard shortcuts.
     */
    void setupShortcuts();

    // Action accessors
    QAction* importAction() const { return m_importAct; }
    QAction* toolbarImportAction() const { return m_toolbarImportAct; }
    QAction* createVideoAction() const { return m_createVideoAct; }
    QAction* previewFitAction() const { return m_previewFitAct; }
    QMenu* recentDatabaseMenu() const { return m_recentDbMenu; }

    // Toolbar widget accessors
    QComboBox* athleteCombo() const { return m_athleteCombo; }

signals:
    /// @brief Emitted when import action is triggered.
    void importTriggered();

    /// @brief Emitted when create video action is triggered.
    void createVideoTriggered();

    /// @brief Emitted when preview FIT action is triggered.
    void previewFitTriggered();

    /// @brief Emitted when database management is requested.
    void openDatabaseRequested();

    /// @brief Emitted when create database is requested.
    void createDatabaseRequested();

    /// @brief Emitted when athlete management is requested.
    void manageAthletesRequested();

    /// @brief Emitted when settings dialog is requested.
    void openSettingsRequested();

    /// @brief Emitted when about dialog is requested.
    void showAboutRequested();

    /// @brief Emitted when detect climbs is requested.
    void detectClimbsRequested();

    /// @brief Emitted when detect intervals is requested.
    void detectIntervalsRequested();

    /// @brief Emitted when athlete selection changes.
    void athleteSelectionChanged(int athleteId);

private:
    /// @brief Helper to create File menu.
    void createFileMenu();

    /// @brief Helper to create Edit menu.
    void createEditMenu();

    /// @brief Helper to create View menu.
    void createViewMenu();

    /// @brief Helper to create Analysis menu.
    void createAnalysisMenu();

    /// @brief Helper to create Tools menu.
    void createToolsMenu();

    /// @brief Helper to create Help menu.
    void createHelpMenu();

    QMainWindow* m_mainWindow = nullptr;

    // File menu actions
    QAction* m_importAct = nullptr;
    QAction* m_previewFitAct = nullptr;
    QAction* m_openDatabaseAct = nullptr;
    QAction* m_createDatabaseAct = nullptr;
    QMenu* m_recentDbMenu = nullptr;
    QAction* m_backupDatabaseAct = nullptr;
    QAction* m_exitAct = nullptr;

    // Edit menu actions
    QAction* m_manageAthletesAct = nullptr;
    QAction* m_openSettingsAct = nullptr;

    // View menu actions
    QAction* m_fullscreenAct = nullptr;

    // Analysis menu actions
    QAction* m_detectClimbsAct = nullptr;
    QAction* m_detectIntervalsAct = nullptr;
    QAction* m_revertClimbAct = nullptr;

    // Tools menu actions
    QAction* m_createVideoAct = nullptr;

    // Help menu actions
    QAction* m_aboutAct = nullptr;

    // Toolbar actions
    QAction* m_toolbarImportAct = nullptr;
    QAction* m_toolbarSearchAct = nullptr;
    QComboBox* m_athleteCombo = nullptr;
};
