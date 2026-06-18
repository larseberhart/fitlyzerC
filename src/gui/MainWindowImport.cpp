// SPDX-License-Identifier: GPL-3

#include "MainWindow.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QMimeData>
#include <QStatusBar>
#include <QTimer>
#include <QUrl>
#include <QUuid>

void MainWindow::importFiles(const QStringList& filePaths)
{
    importFilesInternal(filePaths, true, QStringLiteral("Manual import"));
}

void MainWindow::importFilesInternal(const QStringList& filePaths,
                                     bool showResultDialog,
                                     const QString& sourceLabel)
{
    const QStringList prepared = prepareImportFileBatch(filePaths);
    if (prepared.isEmpty())
        return;

    enqueueImportBatch(prepared, showResultDialog, sourceLabel);
}

QStringList MainWindow::prepareImportFileBatch(const QStringList& filePaths)
{
    QStringList deduped = filePaths;
    deduped.removeDuplicates();
    if (deduped.isEmpty())
        return {};

    if (!ensureImportReady())
    {
        updateImportAvailability();
        return {};
    }

    if (!m_importQueue)
        return {};

    return deduped;
}

void MainWindow::enqueueImportBatch(const QStringList& filePaths,
                                    bool showResultDialog,
                                    const QString& sourceLabel)
{
    if (!m_importQueue || filePaths.isEmpty())
        return;

    const QString batchId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ImportBatchSummary summary;
    summary.queued = filePaths.size();
    summary.showResultDialog = showResultDialog;
    summary.sourceLabel = sourceLabel;
    m_importBatchSummaries.insert(batchId, summary);

    const QString importSource = showResultDialog
        ? QStringLiteral("manual_import")
        : QStringLiteral("directory_import");

    for (const QString& filePath : filePaths)
    {
        m_importQueue->enqueue(filePath,
                               QString::number(m_currentAthleteId),
                               batchId,
                               importSource);
    }

    statusBar()->showMessage(importQueuedStatusMessage(filePaths.size()), 2500);
}

QString MainWindow::importQueuedStatusMessage(int queuedCount) const
{
    return QString("Queued %1 FIT file%2 for background import.")
        .arg(queuedCount)
        .arg(queuedCount == 1 ? QString() : QStringLiteral("s"));
}

void MainWindow::scheduleActivityBrowserRefresh()
{
    if (m_importRefreshTimer)
        m_importRefreshTimer->start();
}

void MainWindow::finalizeImportBatches()
{
    if (m_importBatchSummaries.isEmpty())
        return;

    for (auto it = m_importBatchSummaries.begin(); it != m_importBatchSummaries.end(); ++it)
    {
        const ImportBatchSummary& summary = it.value();
        const QString label = summary.sourceLabel.isEmpty()
            ? QStringLiteral("Import")
            : summary.sourceLabel;

        if (summary.showResultDialog)
        {
            QString text = QString("%1 files selected\n\nImported: %2\nDuplicates: %3\nFailed: %4")
                .arg(summary.queued)
                .arg(summary.imported)
                .arg(summary.duplicates)
                .arg(summary.failed);

            if (!summary.errors.isEmpty())
                text += QString("\n\nErrors:\n%1").arg(summary.errors.join('\n'));

            QMessageBox::information(this, "Import Results", text);
        }
        else
        {
            statusBar()->showMessage(
                QString("%1: imported %2, duplicates %3, failed %4")
                    .arg(label)
                    .arg(summary.imported)
                    .arg(summary.duplicates)
                    .arg(summary.failed),
                5000);
        }
    }

    m_importBatchSummaries.clear();
}

QStringList MainWindow::fitFilesFromMimeData(const QMimeData* mimeData) const
{
    QStringList files;
    if (!mimeData || !mimeData->hasUrls())
        return files;

    for (const QUrl& url : mimeData->urls())
    {
        if (!url.isLocalFile())
            continue;
        const QString path = url.toLocalFile();
        if (QFileInfo(path).suffix().compare("fit", Qt::CaseInsensitive) == 0)
            files << path;
    }

    files.removeDuplicates();
    return files;
}

void MainWindow::importActivities()
{
    const QStringList files = QFileDialog::getOpenFileNames(
            this, "Import Activities",
            m_lastOpenDir,
            "FIT Files (*.fit);;All Files (*)");

    if (files.isEmpty()) return;
    m_lastOpenDir = QFileInfo(files.front()).absolutePath();
    importFiles(files);
}

void MainWindow::previewFitFile()
{
    const QString fileName = QFileDialog::getOpenFileName(
            this, "Preview FIT File",
            m_lastOpenDir,
            "FIT Files (*.fit);;All Files (*)");
    if (fileName.isEmpty()) return;
    m_lastOpenDir = QFileInfo(fileName).absolutePath();

    QString err;
    if (!m_controller->loadFile(fileName, err))
        QMessageBox::critical(this, "Preview Failed", err);
}
