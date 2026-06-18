// SPDX-License-Identifier: GPL-3

#include "MainWindow.h"

#include <QDate>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QStatusBar>

#include "analysis/PowerCurve.h"
#include "core/settings/DateFormatter.h"
#include "database/AthleteRepository.h"
#include "charts/RideChartWidget.h"
#include "controllers/ChartController.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace
{
static constexpr int kChartPresetCustom = 0;
static constexpr int kChartPresetPowerAnalysis = 1;
static constexpr int kChartPresetHeartRateFocus = 2;
static constexpr int kChartPresetRaceReview = 3;
}

void MainWindow::resetAllZoom()
{
    const std::initializer_list<RideChartWidget*> all =
        { m_powerChart, m_hrChart, m_cadenceChart, m_speedChart, m_altitudeChart };
    for (auto* c : all)
        c->fitToData();
}

void MainWindow::increaseChartHeight()
{
    m_chartHeight += 40;
    applyChartHeight();
}

void MainWindow::decreaseChartHeight()
{
    m_chartHeight = std::max(120, m_chartHeight - 40);
    applyChartHeight();
}

void MainWindow::applyChartHeight()
{
    const std::initializer_list<RideChartWidget*> all =
        { m_powerChart, m_hrChart, m_cadenceChart, m_speedChart, m_altitudeChart };

    int visibleCount = 0;
    for (auto* c : all)
    {
        c->setFixedHeight(m_chartHeight);
        if (c->isVisible())
            ++visibleCount;
    }

    if (!m_chartsContainer)
        return;

    const int spacing = m_chartsLayout ? m_chartsLayout->spacing() : 4;
    const int totalH = visibleCount * m_chartHeight
        + std::max(0, visibleCount - 1) * spacing;
    m_chartsContainer->setFixedHeight(std::max(totalH, 1));

    if (m_chartsContainer)
        m_chartsContainer->setMaximumWidth(QWIDGETSIZE_MAX);
}

void MainWindow::onWorkoutLoaded()
{
    updateStatsLabel();
    updateChartAnalysisEmptyStates();

    const bool hasPower = m_controller->statistics().maximumPower > 0.0;
    if (m_chartController)
    {
        syncChartContextToController(true);
        m_chartController->handleWorkoutLoaded();
    }

    if (hasPower)
        updateIntervals();

    updateClimbingTab();
    refreshMapRideDataFromCurrentState();
}

void MainWindow::applyChartPreset(int presetId)
{
    if (!m_colorMetricCombo || !m_powerSmoothingCombo || !m_autoSmoothingCheck)
        return;

    if (presetId == kChartPresetCustom)
        return;

    QSignalBlocker colorBlock(m_colorMetricCombo);
    QSignalBlocker smoothingBlock(m_powerSmoothingCombo);
    QSignalBlocker autoBlock(m_autoSmoothingCheck);

    switch (presetId)
    {
        case kChartPresetPowerAnalysis:
            m_colorMetricCombo->setCurrentIndex(
                m_colorMetricCombo->findData(static_cast<int>(ColorMetric::Power)));
            m_powerSmoothingCombo->setCurrentIndex(m_powerSmoothingCombo->findData(5));
            m_autoSmoothingCheck->setChecked(false);
            m_showPower->setChecked(true);
            m_showHR->setChecked(true);
            m_showCadence->setChecked(true);
            m_showSpeed->setChecked(false);
            m_showAltitude->setChecked(false);
            break;

        case kChartPresetHeartRateFocus:
            m_colorMetricCombo->setCurrentIndex(
                m_colorMetricCombo->findData(static_cast<int>(ColorMetric::HeartRate)));
            m_powerSmoothingCombo->setCurrentIndex(m_powerSmoothingCombo->findData(10));
            m_autoSmoothingCheck->setChecked(true);
            m_showPower->setChecked(true);
            m_showHR->setChecked(true);
            m_showCadence->setChecked(false);
            m_showSpeed->setChecked(false);
            m_showAltitude->setChecked(false);
            break;

        case kChartPresetRaceReview:
            m_colorMetricCombo->setCurrentIndex(
                m_colorMetricCombo->findData(static_cast<int>(ColorMetric::Power)));
            m_powerSmoothingCombo->setCurrentIndex(m_powerSmoothingCombo->findData(30));
            m_autoSmoothingCheck->setChecked(true);
            m_showPower->setChecked(true);
            m_showHR->setChecked(true);
            m_showCadence->setChecked(true);
            m_showSpeed->setChecked(true);
            m_showAltitude->setChecked(true);
            break;

        default:
            return;
    }

    m_powerSmoothingCombo->setEnabled(!m_autoSmoothingCheck->isChecked());
    applyChartAndMapUpdates(true, true);
}

double MainWindow::estimatedFtpFromCurrentRide() const
{
    const double best20m = PowerCurve::bestMeanPower(m_controller->rideData(), 20.0 * 60.0);
    if (best20m <= 0.0)
        return 0.0;
    return best20m * 0.95;
}

void MainWindow::applyEstimatedFtpForCurrentAthlete()
{
    if (!m_dbManager.isOpen() || m_currentAthleteId <= 0)
        return;

    const int estimated = static_cast<int>(std::round(estimatedFtpFromCurrentRide()));
    if (estimated <= 0)
    {
        QMessageBox::information(this, "Estimated FTP", "Insufficient power data to estimate FTP.");
        return;
    }

    auto db = m_dbManager.database();
    AthleteRepository repo(db);
    Athlete athlete = repo.getAthlete(m_currentAthleteId);
    if (!athlete.isValid())
        return;

    athlete.ftpWatts = estimated;
    if (!repo.updateAthlete(athlete))
    {
        QMessageBox::critical(this, "FTP Update", "Failed to update athlete FTP.");
        return;
    }

    FtpEntry entry;
    entry.athleteId = m_currentAthleteId;
    entry.ftpWatts = estimated;
    entry.effectiveFrom = DateFormatter::toIsoDate(QDate::currentDate());
    entry.notes = "Estimated from best 20-minute power (95%).";
    repo.addFtpEntry(entry);

    m_controller->setCurrentAthlete(m_currentAthleteId);
    updateStatsLabel();
    statusBar()->showMessage(QString("Estimated FTP applied: %1 W").arg(estimated), 3500);
}
