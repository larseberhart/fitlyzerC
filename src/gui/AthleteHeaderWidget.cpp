// SPDX-License-Identifier: GPL-3


#include "AthleteHeaderWidget.h"

#include <QHBoxLayout>
#include <QLabel>

AthleteHeaderWidget::AthleteHeaderWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(40);
    setStyleSheet(
        "AthleteHeaderWidget {"
        "  background: #1e3a5f;"
        "  border-bottom: 1px solid #14274a;"
        "}");

    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(12, 0, 12, 0);
    row->setSpacing(0);

    // ── Left: athlete name ────────────────────────────────────────────────
    m_nameLabel = new QLabel("No athlete selected", this);
    m_nameLabel->setStyleSheet(
        "font-size: 14px; font-weight: 700; color: #0c4a6e;"
        " min-width: 160px;");

    // ── Middle: FTP / activities / last ride ──────────────────────────────
    m_statsLabel = new QLabel("FTP 0 W   Activities 0   Last -", this);
    m_statsLabel->setAlignment(Qt::AlignCenter);
    m_statsLabel->setStyleSheet(
        "font-size: 12px; color: #374151;");

    // ── Right: per-ride metrics ──────────────────────────────────────────────────────────────────────
    m_metricsLabel = new QLabel("", this);
    m_metricsLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_metricsLabel->setStyleSheet(
        "font-size: 12px; font-weight: 600; color: #1f2937;"
        " min-width: 160px;");

    row->addWidget(m_nameLabel, 0);
    row->addWidget(m_statsLabel, 1);
    row->addWidget(m_metricsLabel, 0);
}

void AthleteHeaderWidget::setSummary(const QString& athleteName,
                                     int ftpWatts,
                                     int activityCount,
                                     const QString& lastActivityDate)
{
    m_nameLabel->setText(athleteName.isEmpty() ? QString("No athlete selected") : athleteName);
    m_statsLabel->setText(
        QString("FTP %1 W  \u00b7  %2 activities  \u00b7  Last %3")
            .arg(ftpWatts)
            .arg(activityCount)
            .arg(lastActivityDate));
}

void AthleteHeaderWidget::setRideMetrics(const QString& metricsText)
{
    m_metricsLabel->setText(metricsText);
}