// SPDX-License-Identifier: GPL-3

/**
 * @file ActivityBrowser.cpp
 * @brief User interface component for ActivityBrowser.
 *
 * Defines dialogs, widgets, controllers, and UI workflows used by the FitlyzerC desktop application.
 *
 * Responsibilities:
 * - Provide interactive user interface behavior and presentation
 *
 * @author Lars EBERHART
 */

#include "ActivityBrowser.h"
#include "EditNotesDialog.h"
#include "core/settings/DateFormatter.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QMenu>
#include <QMessageBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUrl>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QDate>

namespace
{
constexpr int kNotesRole = Qt::UserRole + 1;
constexpr int kWeatherRole = Qt::UserRole + 2;
constexpr int kIsoDateRole = Qt::UserRole + 3;

class ActivityDateTableItem : public QTableWidgetItem
{
public:
    using QTableWidgetItem::QTableWidgetItem;

    bool operator<(const QTableWidgetItem& other) const override
    {
        const QString lhsIso = data(kIsoDateRole).toString();
        const QString rhsIso = other.data(kIsoDateRole).toString();

        const QDate lhsDate = QDate::fromString(lhsIso, Qt::ISODate);
        const QDate rhsDate = QDate::fromString(rhsIso, Qt::ISODate);
        if (lhsDate.isValid() && rhsDate.isValid())
            return lhsDate < rhsDate;

        // Fall back to ISO lexical compare for partial values and then display text.
        const int isoCmp = QString::compare(lhsIso, rhsIso, Qt::CaseInsensitive);
        if (isoCmp != 0)
            return isoCmp < 0;

        return QTableWidgetItem::operator<(other);
    }
};
}

static QString fmtDurAB(double seconds)
{
    const int total = static_cast<int>(seconds);
    const int h = total / 3600;
    const int m = (total % 3600) / 60;
    const int s = total % 60;
    return h > 0
        ? QString("%1:%2:%3").arg(h).arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0'))
        : QString("%1:%2").arg(m).arg(s,2,10,QChar('0'));
}

ActivityBrowser::ActivityBrowser(DatabaseManager* dbManager, QWidget* parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
{
    setAcceptDrops(true);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* filters = new QHBoxLayout;
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search activities, notes, or dates");
    m_searchEdit->setClearButtonEnabled(true);
    m_rangeFilter = new QComboBox(this);
    m_rangeFilter->addItems({"All", "Last 7 Days", "Last 30 Days", "This Year"});
    filters->addWidget(m_searchEdit, 1);
    filters->addWidget(m_rangeFilter);
    layout->addLayout(filters);

    m_emptyStateLabel = new QLabel(this);
    m_emptyStateLabel->setWordWrap(true);
    m_emptyStateLabel->setAlignment(Qt::AlignCenter);
    m_emptyStateLabel->setText(
        "No activities yet.\n\nDrop .fit files here or use Import Activities to add rides.");
    m_emptyStateLabel->setStyleSheet(
        "QLabel { background: #f8fafc; color: #475569; border: 1px dashed #cbd5e1;"
        " padding: 20px; border-radius: 8px; font-size: 14px; }");
    layout->addWidget(m_emptyStateLabel);

    m_table = new QTableWidget(0, 8);
    m_table->setHorizontalHeaderLabels({
        "Date", "File", "Duration", "Distance (km)",
        "Avg W", "NP", "Avg HR", "Elevation (m)"
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);

    layout->addWidget(m_table);

        connect(m_searchEdit, &QLineEdit::textChanged,
            this, [this] { applyFilters(); });
        connect(m_rangeFilter, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int) { applyFilters(); });

    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, [this](int row, int)
    {
        auto* item = m_table->item(row, 0);
        if (!item) return;
        const int actId = item->data(Qt::UserRole).toInt();
        if (actId > 0)
            emit activitySelected(actId);
    });

    connect(m_table, &QWidget::customContextMenuRequested,
            this, &ActivityBrowser::showContextMenu);

    applyFilters();
}

void ActivityBrowser::focusSearch()
{
    if (!m_searchEdit)
        return;

    m_searchEdit->setFocus();
    m_searchEdit->selectAll();
}

void ActivityBrowser::dragEnterEvent(QDragEnterEvent* event)
{
    if (!event->mimeData()->hasUrls())
        return;

    for (const QUrl& url : event->mimeData()->urls())
    {
        if (!url.isLocalFile())
            continue;
        if (url.toLocalFile().toLower().endsWith(".fit"))
        {
            event->acceptProposedAction();
            return;
        }
    }
}

void ActivityBrowser::dropEvent(QDropEvent* event)
{
    if (!event->mimeData()->hasUrls())
        return;

    QStringList fitFiles;
    for (const QUrl& url : event->mimeData()->urls())
    {
        if (!url.isLocalFile())
            continue;
        const QString path = url.toLocalFile();
        if (QFileInfo(path).suffix().compare("fit", Qt::CaseInsensitive) == 0)
            fitFiles << path;
    }

    fitFiles.removeDuplicates();
    if (fitFiles.isEmpty())
        return;

    emit fitFilesDropped(fitFiles);
    event->acceptProposedAction();
}

void ActivityBrowser::refresh(int athleteId)
{
    m_athleteId = athleteId;
    m_table->setSortingEnabled(false);
    m_table->setRowCount(0);

    if (!m_dbManager || !m_dbManager->isOpen())
    {
        applyFilters();
        return;
    }

    auto db = m_dbManager->database();
    ActivityRepository repo(db);

    for (const Activity& a : repo.listActivities(athleteId))
    {
        const int row = m_table->rowCount();
        m_table->insertRow(row);

        const QString isoDate = !a.startTime.isEmpty() ? a.startTime.left(10) : a.importedAt.left(10);
        const QDate parsedDate = QDate::fromString(isoDate, Qt::ISODate);
        const QString displayDate = parsedDate.isValid() ? DateFormatter::formatDate(parsedDate) : isoDate;

        auto* dateItem = new ActivityDateTableItem(displayDate);
        dateItem->setData(Qt::UserRole, a.id);
        dateItem->setData(kNotesRole, a.notes);
        dateItem->setData(kWeatherRole, a.weatherNotes);
        dateItem->setData(kIsoDateRole, isoDate);
        m_table->setItem(row, 0, dateItem);

        m_table->setItem(row, 1, new QTableWidgetItem(a.fileName));
        m_table->setItem(row, 2, new QTableWidgetItem(fmtDurAB(a.durationSec)));
        m_table->setItem(row, 3, new QTableWidgetItem(
            a.distanceM > 0 ? QString::number(a.distanceM / 1000.0, 'f', 1) : ""));
        m_table->setItem(row, 4, new QTableWidgetItem(
            a.avgPower > 0 ? QString::number(a.avgPower, 'f', 0) : ""));
        m_table->setItem(row, 5, new QTableWidgetItem(
            a.normalizedPower > 0 ? QString::number(a.normalizedPower, 'f', 0) : ""));
        m_table->setItem(row, 6, new QTableWidgetItem(
            a.avgHr > 0 ? QString::number(a.avgHr, 'f', 0) : ""));
        m_table->setItem(row, 7, new QTableWidgetItem(
            a.elevationGain > 0 ? QString::number(a.elevationGain, 'f', 0) : ""));
    }

    m_table->setSortingEnabled(true);
    m_table->resizeColumnsToContents();
    applyFilters();
}

void ActivityBrowser::applyFilters()
{
    if (!m_table)
        return;

    const QString term = m_searchEdit ? m_searchEdit->text().trimmed().toLower() : QString();
    const QString range = m_rangeFilter ? m_rangeFilter->currentText() : QString("All");
    const QDate today = QDate::currentDate();

    int visibleRows = 0;
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        bool matchesText = term.isEmpty();
        if (!matchesText)
        {
            QString rowText;
            for (int col = 0; col < m_table->columnCount(); ++col)
            {
                auto* item = m_table->item(row, col);
                if (item)
                    rowText += item->text().toLower() + " ";
            }
            auto* dateItem = m_table->item(row, 0);
            if (dateItem)
                rowText += dateItem->data(kNotesRole).toString().toLower() + " ";
            matchesText = rowText.contains(term);
        }

        bool matchesDate = true;
        auto* dateItem = m_table->item(row, 0);
        const QDate rowDate = dateItem
            ? QDate::fromString(dateItem->data(kIsoDateRole).toString(), Qt::ISODate)
            : QDate();

        if (rowDate.isValid() && range != "All")
        {
            if (range == "Last 7 Days")
                matchesDate = rowDate >= today.addDays(-7);
            else if (range == "Last 30 Days")
                matchesDate = rowDate >= today.addDays(-30);
            else if (range == "This Year")
                matchesDate = rowDate.year() == today.year();
        }

        const bool visible = matchesText && matchesDate;
        m_table->setRowHidden(row, !visible);
        if (visible)
            ++visibleRows;
    }

    if (m_emptyStateLabel)
    {
        const bool hasDatabase = m_dbManager && m_dbManager->isOpen();
        if (!hasDatabase)
        {
            m_emptyStateLabel->setText("Open or create a database to browse activities.");
        }
        else if (m_table->rowCount() == 0)
        {
            m_emptyStateLabel->setText(
                "No activities yet.\n\nDrop .fit files here or use Import Activities to add rides.");
        }
        else if (visibleRows == 0)
        {
            m_emptyStateLabel->setText("No activities match the current search or date filter.");
        }

        m_emptyStateLabel->setVisible(!hasDatabase || m_table->rowCount() == 0 || visibleRows == 0);
    }

    m_table->setVisible(m_table->rowCount() > 0);
}

void ActivityBrowser::showContextMenu(const QPoint& pos)
{
    const int row = m_table->rowAt(pos.y());
    if (row < 0) return;
    auto* item = m_table->item(row, 0);
    if (!item) return;
    const int actId = item->data(Qt::UserRole).toInt();
    if (actId <= 0) return;

    QMenu menu(this);
    auto* loadAct   = menu.addAction("Load Activity");
    auto* notesAct  = menu.addAction("Edit Notes...");
    menu.addSeparator();
    auto* deleteAct = menu.addAction("Delete Activity");

    const QAction* chosen = menu.exec(m_table->viewport()->mapToGlobal(pos));
    if (chosen == loadAct)
        emit activitySelected(actId);
    else if (chosen == notesAct)
        editNotes(actId);
    else if (chosen == deleteAct)
        deleteActivity(actId);
}

void ActivityBrowser::editNotes(int activityId)
{
    if (!m_dbManager || !m_dbManager->isOpen()) return;

    // Retrieve current notes from the row's stored data
    QString notes, weather;
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        auto* item = m_table->item(row, 0);
        if (item && item->data(Qt::UserRole).toInt() == activityId)
        {
            notes   = item->data(kNotesRole).toString();
            weather = item->data(kWeatherRole).toString();
            break;
        }
    }

    EditNotesDialog dlg(notes, weather, this);
    if (dlg.exec() != QDialog::Accepted) return;

    auto db = m_dbManager->database();
    ActivityRepository repo(db);
    if (repo.updateNotes(activityId, dlg.notes(), dlg.weatherNotes()))
        refresh(m_athleteId);   // reload to show updated data
    else
        QMessageBox::critical(this, "Error", "Failed to save notes.");
}

void ActivityBrowser::deleteActivity(int activityId)
{
    const auto ans = QMessageBox::question(
        this, "Delete Activity",
        "Delete this activity and all its samples?\nThis cannot be undone.",
        QMessageBox::Yes | QMessageBox::No);
    if (ans != QMessageBox::Yes) return;

    if (!m_dbManager || !m_dbManager->isOpen()) return;

    auto db = m_dbManager->database();
    ActivityRepository repo(db);
    if (repo.deleteActivity(activityId))
    {
        refresh(m_athleteId);
        emit activityDeleted(activityId);
    }
    else
        QMessageBox::critical(this, "Error", "Failed to delete activity.");
}