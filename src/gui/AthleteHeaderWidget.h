// SPDX-License-Identifier: GPL-3


#pragma once

#include <QWidget>

class QLabel;

/**
 * @brief Header bar displaying current athlete and ride metrics.
 *
 * Shows athlete name, FTP, activity count, and current ride metrics (NP, IF, TSS).
 */
class AthleteHeaderWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AthleteHeaderWidget(QWidget* parent = nullptr);

    void setSummary(const QString& athleteName,
                    int ftpWatts,
                    int activityCount,
                    const QString& lastActivityDate);

    void setRideMetrics(const QString& metricsText);

private:
    QLabel* m_nameLabel    = nullptr;   // left: athlete name
    QLabel* m_statsLabel   = nullptr;   // middle: FTP / activities / last ride
    QLabel* m_metricsLabel = nullptr;   // right: NP / IF / TSS …
};;