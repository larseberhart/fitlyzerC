// SPDX-License-Identifier: GPL-3

#include "MainWindow.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QMimeData>
#include <QStandardPaths>
#include <QTimer>

#include <algorithm>

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (!event)
        return;

    if (!fitFilesFromMimeData(event->mimeData()).isEmpty())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    if (!event)
        return;

    const QStringList files = fitFilesFromMimeData(event->mimeData());
    if (files.isEmpty())
        return;

    importFiles(files);
    event->acceptProposedAction();
}

void MainWindow::configureFolderWatcher()
{
    if (!m_watcher)
    {
        m_watcher = new QFileSystemWatcher(this);
        connect(m_watcher, &QFileSystemWatcher::directoryChanged,
                this, &MainWindow::onWatchDirectoryChanged);
    }

    if (!m_watchRescanTimer)
    {
        m_watchRescanTimer = new QTimer(this);
        m_watchRescanTimer->setSingleShot(true);
        m_watchRescanTimer->setInterval(700);
        connect(m_watchRescanTimer, &QTimer::timeout, this, [this]
        {
            scanWatchDirectories(false);
        });
    }

    const QStringList current = m_watcher->directories();
    if (!current.isEmpty())
        m_watcher->removePaths(current);

    m_knownWatchedFitFiles.clear();
    if (!m_watchFolderEnabled)
        return;

    const QStringList dirs = monitoredDirectories();
    QStringList validDirs;
    for (const QString& dirPath : dirs)
    {
        const QFileInfo info(dirPath);
        if (info.exists() && info.isDir())
            validDirs << info.absoluteFilePath();
    }

    validDirs.removeDuplicates();
    if (!validDirs.isEmpty())
    {
        m_watcher->addPaths(validDirs);
        scanWatchDirectories(true);
    }
}

QStringList MainWindow::monitoredDirectories() const
{
    QStringList dirs;
    const QString downloads = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    const QString primary = m_watchFolderPath.trimmed().isEmpty() ? downloads : m_watchFolderPath;
    if (!primary.isEmpty())
        dirs << QDir::cleanPath(primary);

    const QString home = QDir::homePath();
    const QStringList candidates = {
        QDir(home).filePath("Downloads/Garmin"),
        QDir(home).filePath("Downloads/Garmin Export"),
        QDir(home).filePath("Garmin/Exports")
    };
    for (const QString& candidate : candidates)
    {
        if (QFileInfo::exists(candidate))
            dirs << QDir::cleanPath(candidate);
    }

    dirs.removeDuplicates();
    return dirs;
}

void MainWindow::scanWatchDirectories(bool initialScan)
{
    if (!m_watchFolderEnabled)
        return;

    const QStringList dirs = monitoredDirectories();
    QSet<QString> currentSet;

    for (const QString& dirPath : dirs)
    {
        QDirIterator it(
            dirPath,
            QStringList() << "*.fit" << "*.FIT",
            QDir::Files,
            QDirIterator::Subdirectories);
        while (it.hasNext())
            currentSet.insert(QDir::cleanPath(it.next()));
    }

    if (initialScan)
    {
        m_knownWatchedFitFiles = currentSet;
        return;
    }

    QStringList newFiles;
    for (const QString& path : currentSet)
    {
        if (!m_knownWatchedFitFiles.contains(path))
            newFiles << path;
    }

    m_knownWatchedFitFiles = currentSet;
    if (newFiles.isEmpty())
        return;

    std::sort(newFiles.begin(), newFiles.end());
    importFilesInternal(newFiles, false, QStringLiteral("Folder watcher"));
}

void MainWindow::onWatchDirectoryChanged(const QString& path)
{
    (void)path;
    if (!m_watchRescanTimer)
        return;
    m_watchRescanTimer->start();
}
