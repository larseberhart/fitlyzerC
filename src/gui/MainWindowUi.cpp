// SPDX-License-Identifier: GPL-3

#include "MainWindow.h"

#include <QAction>
#include <QComboBox>
#include <QEvent>
#include <QKeySequence>
#include <QLabel>
#include <QStatusBar>
#include <QStyle>
#include <QToolBar>

#include "ActivityBrowser.h"
#include "ImportStatusWidget.h"
#include "NavigationSidebar.h"

#include <cmath>

void MainWindow::buildToolbar()
{
    auto* tb = addToolBar("Main");
    tb->setMovable(false);
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    m_toolbarImportAct = new QAction("Import", this);
    m_toolbarImportAct->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    m_toolbarImportAct->setEnabled(false);
    connect(m_toolbarImportAct, &QAction::triggered, this, &MainWindow::importActivities);
    tb->addAction(m_toolbarImportAct);

    auto* athleteManageAct = new QAction("Athletes", this);
    athleteManageAct->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    connect(athleteManageAct, &QAction::triggered, this, &MainWindow::manageAthletes);
    tb->addAction(athleteManageAct);

    auto* calendarAct = new QAction("Calendar", this);
    calendarAct->setIcon(style()->standardIcon(QStyle::SP_FileDialogContentsView));
    connect(calendarAct, &QAction::triggered, this, [this]
    {
        if (m_navigationSidebar)
            m_navigationSidebar->setCurrentPage(NavigationSidebar::Page::Calendar);
    });
    tb->addAction(calendarAct);

    m_toolbarSearchAct = new QAction("Search", this);
    m_toolbarSearchAct->setIcon(style()->standardIcon(QStyle::SP_FileDialogContentsView));
    connect(m_toolbarSearchAct, &QAction::triggered, this, [this]
    {
        if (m_navigationSidebar)
            m_navigationSidebar->setCurrentPage(NavigationSidebar::Page::Activities);
        if (m_activityBrowser)
            m_activityBrowser->focusSearch();
    });
    tb->addAction(m_toolbarSearchAct);

    tb->addSeparator();
    tb->addWidget(new QLabel("Current Athlete:"));

    m_athleteCombo = new QComboBox(tb);
    m_athleteCombo->setMinimumWidth(220);
    connect(m_athleteCombo, &QComboBox::currentIndexChanged,
            this, &MainWindow::onAthleteSelectionChanged);
    tb->addWidget(m_athleteCombo);

    tb->addSeparator();
    tb->addWidget(new QLabel(" "));

    m_importStatusWidget = new ImportStatusWidget(tb);
    if (m_importProgressModel)
        m_importStatusWidget->setModel(m_importProgressModel);
    tb->addWidget(m_importStatusWidget);
}

void MainWindow::buildStatusBar()
{
    m_dbStatusLabel = new QLabel("Database: none");
    m_athleteStatusLabel = new QLabel("Athlete: none");
    m_activityCountLabel = new QLabel("Activities: 0");

    statusBar()->addPermanentWidget(m_dbStatusLabel);
    statusBar()->addPermanentWidget(m_athleteStatusLabel);
    statusBar()->addPermanentWidget(m_activityCountLabel);
}

void MainWindow::setupShortcuts()
{
    auto* fsAct = new QAction(this);
    fsAct->setShortcut(Qt::Key_F11);
    connect(fsAct, &QAction::triggered, this, [this]
    {
        isFullScreen() ? showNormal() : showFullScreen();
    });
    addAction(fsAct);

    auto* resetAct = new QAction(this);
    resetAct->setShortcut(QKeySequence("Ctrl+0"));
    connect(resetAct, &QAction::triggered, this, &MainWindow::resetAllZoom);
    addAction(resetAct);

    auto* undoAct = new QAction(this);
    undoAct->setShortcut(QKeySequence::Undo);
    connect(undoAct, &QAction::triggered, m_undoManager, &UndoManager::undo);
    addAction(undoAct);

    auto* redoAct = new QAction(this);
    redoAct->setShortcut(QKeySequence::Redo);
    connect(redoAct, &QAction::triggered, m_undoManager, &UndoManager::redo);
    addAction(redoAct);

    if (m_intervalsTable)
    {
        auto* prevIntervalAct = new QAction(m_intervalsTable);
        prevIntervalAct->setShortcut(Qt::Key_Up);
        prevIntervalAct->setShortcutContext(Qt::WidgetShortcut);
        connect(prevIntervalAct, &QAction::triggered, this, [this]
        {
            selectIntervalRowOffset(-1);
        });
        m_intervalsTable->addAction(prevIntervalAct);

        auto* nextIntervalAct = new QAction(m_intervalsTable);
        nextIntervalAct->setShortcut(Qt::Key_Down);
        nextIntervalAct->setShortcutContext(Qt::WidgetShortcut);
        connect(nextIntervalAct, &QAction::triggered, this, [this]
        {
            selectIntervalRowOffset(1);
        });
        m_intervalsTable->addAction(nextIntervalAct);
    }
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (m_chartScroll && watched == m_chartScroll->viewport()
            && event->type() == QEvent::Resize)
    {
        applyChartHeight();
    }

    if (m_chartMapSplit && watched == m_chartMapSplit && event->type() == QEvent::Resize)
    {
        const QList<int> sizes = m_chartMapSplit->sizes();
        if (sizes.size() == 2 && std::abs(sizes[0] - sizes[1]) > 1)
        {
            const int totalWidth = sizes[0] + sizes[1];
            if (totalWidth > 0)
                m_chartMapSplit->setSizes({ totalWidth / 2, totalWidth / 2 });
        }
    }

    return QMainWindow::eventFilter(watched, event);
}
