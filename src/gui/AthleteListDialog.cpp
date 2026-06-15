// SPDX-License-Identifier: GPL-3

/**
 * @file AthleteListDialog.cpp
 * @brief User interface component for AthleteListDialog.
 *
 * Defines dialogs, widgets, controllers, and UI workflows used by the FitlyzerC desktop application.
 *
 * Responsibilities:
 * - Provide interactive user interface behavior and presentation
 *
 * @author Lars EBERHART
 */

#include "AthleteListDialog.h"
#include "AthleteDialog.h"
#include "core/settings/DateFormatter.h"

#include <QDate>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

AthleteListDialog::AthleteListDialog(AthleteRepository& repo, QWidget* parent)
    : QDialog(parent)
    , m_repo(repo)
{
    setWindowTitle("Athletes");
    setMinimumSize(460, 300);

    auto* mainLayout = new QVBoxLayout(this);

    m_table = new QTableWidget(0, 3);
    m_table->setHorizontalHeaderLabels({"Name", "Date of Birth", "Email"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    mainLayout->addWidget(m_table);

    // ── Buttons ───────────────────────────────────────────────────────────
    auto* btnBar    = new QHBoxLayout;
    auto* newBtn    = new QPushButton("New");
    m_editBtn       = new QPushButton("Edit");
    m_deleteBtn     = new QPushButton("Delete");
    m_selectBtn     = new QPushButton("Select");
    m_selectBtn->setDefault(true);

    btnBar->addWidget(newBtn);
    btnBar->addWidget(m_editBtn);
    btnBar->addWidget(m_deleteBtn);
    btnBar->addStretch();
    btnBar->addWidget(m_selectBtn);
    mainLayout->addLayout(btnBar);

    auto* closeBtn  = new QDialogButtonBox(QDialogButtonBox::Close);
    mainLayout->addWidget(closeBtn);

    connect(newBtn,      &QPushButton::clicked, this, &AthleteListDialog::newAthlete);
    connect(m_editBtn,   &QPushButton::clicked, this, &AthleteListDialog::editAthlete);
    connect(m_deleteBtn, &QPushButton::clicked, this, &AthleteListDialog::deleteAthlete);
    connect(m_selectBtn, &QPushButton::clicked, this, &AthleteListDialog::selectAthlete);
    connect(closeBtn,    &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, [this](int, int){ editAthlete(); });
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this]{
        const bool hasSel = m_table->currentRow() >= 0;
        m_editBtn->setEnabled(hasSel);
        m_deleteBtn->setEnabled(hasSel);
        m_selectBtn->setEnabled(hasSel);
    });

    m_editBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);
    m_selectBtn->setEnabled(false);

    refresh();
}

void AthleteListDialog::refresh()
{
    m_table->setRowCount(0);
    for (const Athlete& a : m_repo.listAthletes())
    {
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        auto* nameItem = new QTableWidgetItem(a.fullName());
        nameItem->setData(Qt::UserRole, a.id);
        m_table->setItem(row, 0, nameItem);
        const QDate dob = QDate::fromString(a.dateOfBirth, Qt::ISODate);
        const QString dobText = dob.isValid() ? DateFormatter::formatDate(dob) : a.dateOfBirth;
        m_table->setItem(row, 1, new QTableWidgetItem(dobText));
        m_table->setItem(row, 2, new QTableWidgetItem(a.email));
    }
}

void AthleteListDialog::newAthlete()
{
    AthleteDialog dlg(m_repo, -1, this);
    if (dlg.exec() == QDialog::Accepted)
        refresh();
}

void AthleteListDialog::editAthlete()
{
    const int row = m_table->currentRow();
    if (row < 0) return;
    const int athleteId = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    AthleteDialog dlg(m_repo, athleteId, this);
    if (dlg.exec() == QDialog::Accepted)
        refresh();
}

void AthleteListDialog::deleteAthlete()
{
    const int row = m_table->currentRow();
    if (row < 0) return;
    const int athleteId = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    const QString name  = m_table->item(row, 0)->text();

    const auto ans = QMessageBox::question(
        this, "Delete Athlete",
        QString("Delete \"%1\" and all their activities?").arg(name),
        QMessageBox::Yes | QMessageBox::No);
    if (ans != QMessageBox::Yes) return;

    m_repo.deleteAthlete(athleteId);
    refresh();
}

void AthleteListDialog::selectAthlete()
{
    const int row = m_table->currentRow();
    if (row < 0) return;
    m_selectedId = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    accept();
}