// SPDX-License-Identifier: GPL-3

/**
 * @file AthleteDialog.cpp
 * @brief User interface component for AthleteDialog.
 *
 * Defines dialogs, widgets, controllers, and UI workflows used by the FitlyzerC desktop application.
 *
 * Responsibilities:
 * - Provide interactive user interface behavior and presentation
 *
 * @author Lars EBERHART
 */

#include "AthleteDialog.h"
#include "core/settings/DateFormatter.h"

#include <QDate>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace
{
constexpr int kEntryIdRole = Qt::UserRole;

QString displayDateOrRaw(const QString& raw)
{
    const QDate parsed = QDate::fromString(raw, Qt::ISODate);
    return parsed.isValid() ? DateFormatter::formatDate(parsed) : raw;
}

QString toIsoOrRaw(const QString& text)
{
    const QDate parsed = DateFormatter::parseDate(text);
    return parsed.isValid() ? DateFormatter::toIsoDate(parsed) : text.trimmed();
}
}

AthleteDialog::AthleteDialog(AthleteRepository& repo, int athleteId, QWidget* parent)
    : QDialog(parent)
    , m_repo(repo)
    , m_athleteId(athleteId)
{
    setWindowTitle(athleteId < 0 ? "New Athlete" : "Edit Athlete");
    setMinimumWidth(520);

    auto* mainLayout = new QVBoxLayout(this);

    // ── Basic info ────────────────────────────────────────────────────────
    auto* form = new QFormLayout;
    m_firstName = new QLineEdit;
    m_lastName  = new QLineEdit;
    m_dob       = new QLineEdit;
    m_dob->setPlaceholderText(QString("Date format: %1").arg(DateFormatter::qtDatePattern()));
    m_email     = new QLineEdit;
    m_height    = new QDoubleSpinBox;
    m_ftp       = new QSpinBox;
    m_height->setRange(0.0, 250.0);
    m_height->setSuffix(" cm");
    m_height->setDecimals(1);
    m_height->setSpecialValueText("(not set)");
    m_ftp->setRange(100, 600);
    m_ftp->setValue(250);
    m_ftp->setSuffix(" W");

    form->addRow("First name:",  m_firstName);
    form->addRow("Last name:",   m_lastName);
    form->addRow("Date of birth:", m_dob);
    form->addRow("Email:",       m_email);
    form->addRow("Height:",      m_height);
    form->addRow("FTP:",         m_ftp);
    mainLayout->addLayout(form);

    // ── FTP history ───────────────────────────────────────────────────────
    auto* ftpGroup  = new QGroupBox("FTP History");
    auto* ftpLayout = new QVBoxLayout(ftpGroup);
    m_ftpTable = new QTableWidget(0, 3);
    m_ftpTable->setHorizontalHeaderLabels({"Date", "FTP (W)", "Notes"});
    m_ftpTable->horizontalHeader()->setStretchLastSection(true);
    m_ftpTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ftpTable->setMinimumHeight(120);

    auto* ftpBtnBar = new QHBoxLayout;
    auto* addFtp    = new QPushButton("Add");
    auto* delFtp    = new QPushButton("Delete");
    ftpBtnBar->addWidget(addFtp);
    ftpBtnBar->addWidget(delFtp);
    ftpBtnBar->addStretch();

    ftpLayout->addWidget(m_ftpTable);
    ftpLayout->addLayout(ftpBtnBar);
    mainLayout->addWidget(ftpGroup);

    connect(addFtp, &QPushButton::clicked, this, &AthleteDialog::addFtpRow);
    connect(delFtp, &QPushButton::clicked, this, &AthleteDialog::deleteFtpRow);

    // ── Weight history ────────────────────────────────────────────────────
    auto* wGroup  = new QGroupBox("Weight History");
    auto* wLayout = new QVBoxLayout(wGroup);
    m_weightTable = new QTableWidget(0, 2);
    m_weightTable->setHorizontalHeaderLabels({"Date", "Weight (kg)"});
    m_weightTable->horizontalHeader()->setStretchLastSection(true);
    m_weightTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_weightTable->setMinimumHeight(100);

    auto* wBtnBar = new QHBoxLayout;
    auto* addW    = new QPushButton("Add");
    auto* delW    = new QPushButton("Delete");
    wBtnBar->addWidget(addW);
    wBtnBar->addWidget(delW);
    wBtnBar->addStretch();

    wLayout->addWidget(m_weightTable);
    wLayout->addLayout(wBtnBar);
    mainLayout->addWidget(wGroup);

    connect(addW, &QPushButton::clicked, this, &AthleteDialog::addWeightRow);
    connect(delW, &QPushButton::clicked, this, &AthleteDialog::deleteWeightRow);

    // ── Buttons ───────────────────────────────────────────────────────────
    auto* bbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(bbox);

    connect(bbox, &QDialogButtonBox::accepted, this, &AthleteDialog::onAccepted);
    connect(bbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    if (m_athleteId > 0)
        loadAthlete();
}

void AthleteDialog::loadAthlete()
{
    Athlete a = m_repo.getAthlete(m_athleteId);
    m_firstName->setText(a.firstName);
    m_lastName->setText(a.lastName);
    m_dob->setText(displayDateOrRaw(a.dateOfBirth));
    m_email->setText(a.email);
    m_height->setValue(a.heightCm);
    m_ftp->setValue(a.ftpWatts > 0 ? a.ftpWatts : 250);

    populateFtpTable(m_repo.getFtpHistory(m_athleteId));
    populateWeightTable(m_repo.getWeightHistory(m_athleteId));
}

void AthleteDialog::populateFtpTable(const QList<FtpEntry>& entries)
{
    m_ftpTable->setRowCount(0);
    for (const auto& e : entries)
    {
        const int row = m_ftpTable->rowCount();
        m_ftpTable->insertRow(row);
        m_ftpTable->setItem(row, 0, new QTableWidgetItem(displayDateOrRaw(e.effectiveFrom)));
        m_ftpTable->setItem(row, 1, new QTableWidgetItem(QString::number(e.ftpWatts)));
        m_ftpTable->setItem(row, 2, new QTableWidgetItem(e.notes));
        // Store the entry id in the row's first item
        m_ftpTable->item(row, 0)->setData(kEntryIdRole, e.id);
    }
}

void AthleteDialog::populateWeightTable(const QList<WeightEntry>& entries)
{
    m_weightTable->setRowCount(0);
    for (const auto& e : entries)
    {
        const int row = m_weightTable->rowCount();
        m_weightTable->insertRow(row);
        m_weightTable->setItem(row, 0, new QTableWidgetItem(displayDateOrRaw(e.effectiveFrom)));
        m_weightTable->setItem(row, 1, new QTableWidgetItem(
            QString::number(e.weightKg, 'f', 1)));
        m_weightTable->item(row, 0)->setData(kEntryIdRole, e.id);
    }
}

void AthleteDialog::addFtpRow()
{
    const int row = m_ftpTable->rowCount();
    m_ftpTable->insertRow(row);
    m_ftpTable->setItem(row, 0, new QTableWidgetItem(DateFormatter::formatDate(QDate::currentDate())));
    m_ftpTable->setItem(row, 1, new QTableWidgetItem("250"));
    m_ftpTable->setItem(row, 2, new QTableWidgetItem(""));
    m_ftpTable->item(row, 0)->setData(kEntryIdRole, -1); // new, no db id yet
    m_ftpTable->editItem(m_ftpTable->item(row, 0));
}

void AthleteDialog::deleteFtpRow()
{
    const int row = m_ftpTable->currentRow();
    if (row < 0) return;
    const int entryId = m_ftpTable->item(row, 0)->data(kEntryIdRole).toInt();
    if (entryId > 0 && m_athleteId > 0)
        m_repo.deleteFtpEntry(entryId);
    m_ftpTable->removeRow(row);
}

void AthleteDialog::addWeightRow()
{
    const int row = m_weightTable->rowCount();
    m_weightTable->insertRow(row);
    m_weightTable->setItem(row, 0, new QTableWidgetItem(DateFormatter::formatDate(QDate::currentDate())));
    m_weightTable->setItem(row, 1, new QTableWidgetItem("70.0"));
    m_weightTable->item(row, 0)->setData(kEntryIdRole, -1);
    m_weightTable->editItem(m_weightTable->item(row, 0));
}

void AthleteDialog::deleteWeightRow()
{
    const int row = m_weightTable->currentRow();
    if (row < 0) return;
    const int entryId = m_weightTable->item(row, 0)->data(kEntryIdRole).toInt();
    if (entryId > 0 && m_athleteId > 0)
        m_repo.deleteWeightEntry(entryId);
    m_weightTable->removeRow(row);
}

void AthleteDialog::onAccepted()
{
    if (m_firstName->text().trimmed().isEmpty() ||
        m_lastName->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Missing data",
                             "First name and last name are required.");
        return;
    }

    Athlete a;
    a.id          = m_athleteId;
    a.firstName   = m_firstName->text().trimmed();
    a.lastName    = m_lastName->text().trimmed();
    a.dateOfBirth = toIsoOrRaw(m_dob->text());
    a.email       = m_email->text().trimmed();
    a.heightCm    = m_height->value();
    a.ftpWatts    = m_ftp->value();

    if (m_athleteId < 0)
        m_savedId = m_repo.insertAthlete(a);
    else
    {
        m_repo.updateAthlete(a);
        m_savedId = m_athleteId;
    }

    if (m_savedId < 0)
    {
        QMessageBox::critical(this, "Error", "Failed to save athlete.");
        return;
    }

    // Save new FTP entries (those with id == -1)
    for (int row = 0; row < m_ftpTable->rowCount(); ++row)
    {
        const int entryId = m_ftpTable->item(row, 0)->data(kEntryIdRole).toInt();
        if (entryId < 0)
        {
            FtpEntry e;
            e.athleteId     = m_savedId;
            e.effectiveFrom = toIsoOrRaw(m_ftpTable->item(row, 0)->text());
            e.ftpWatts      = m_ftpTable->item(row, 1)->text().toInt();
            e.notes         = m_ftpTable->item(row, 2)->text();
            m_repo.addFtpEntry(e);
        }
    }

    // Save new weight entries
    for (int row = 0; row < m_weightTable->rowCount(); ++row)
    {
        const int entryId = m_weightTable->item(row, 0)->data(kEntryIdRole).toInt();
        if (entryId < 0)
        {
            WeightEntry e;
            e.athleteId     = m_savedId;
            e.effectiveFrom = toIsoOrRaw(m_weightTable->item(row, 0)->text());
            e.weightKg      = m_weightTable->item(row, 1)->text().toDouble();
            m_repo.addWeightEntry(e);
        }
    }

    accept();
}