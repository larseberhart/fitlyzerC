// SPDX-License-Identifier: GPL-3

/**
 * @file AthleteHeaderWidget.h
 * @brief User interface component for AthleteHeaderWidget.
 *
 * Defines dialogs, widgets, controllers, and UI workflows used by the FitlyzerC desktop application.
 *
 * Responsibilities:
 * - Provide interactive user interface behavior and presentation
 *
 * @author Lars EBERHART
 */

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
    /**
     * @brief Constructs athlete header widget.
     * @param parent Parent widget.
     */
    explicit AthleteHeaderWidget(QWidget* parent = nullptr);

    /**
     * @brief Sets athlete summary information.
     * @param athleteName Athlete name.
     * @param ftpWatts Functional threshold power.
     * @param activityCount Number of activities.
     * @param lastActivityDate Last activity date string.
     */
    void setSummary(const QString& athleteName,
                    int ftpWatts,
                    int activityCount,
                    const QString& lastActivityDate);

    /**
     * @brief Sets current ride metrics display.
     * @param metricsText Formatted metrics text (NP, IF, TSS, etc.).
     */
    void setRideMetrics(const QString& metricsText);

private:
    QLabel* m_nameLabel    = nullptr;   // left: athlete name
    QLabel* m_statsLabel   = nullptr;   // middle: FTP / activities / last ride
    QLabel* m_metricsLabel = nullptr;   // right: NP / IF / TSS …
};;