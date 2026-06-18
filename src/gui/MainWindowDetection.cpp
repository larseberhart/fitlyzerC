// SPDX-License-Identifier: GPL-3

#include "MainWindow.h"

#include <QMessageBox>
#include <QStatusBar>

#include "AboutDialog.h"
#include "EditActivityDialog.h"
#include "analysis/ClimbDetector.h"
#include "analysis/IntervalDetector.h"
#include "database/ActivityRepository.h"
#include "database/ClimbRepository.h"
#include "database/IntervalRepository.h"

#include <cmath>
#include <vector>

void MainWindow::triggerDetectClimbs()
{
    if (!m_dbManager.isOpen() || m_controller->currentActivityId() <= 0)
    {
        QMessageBox::information(this, "Detect Climbs", "No activity loaded.");
        return;
    }

    const auto& ride = m_controller->rideData();
    if (ride.records.empty())
        return;

    auto db = m_dbManager.database();
    ClimbRepository repo(db);
    if (repo.hasLockedClimbs(m_controller->currentActivityId()))
    {
        const int answer = QMessageBox::warning(
            this,
            "Detect Climbs",
            "This activity contains manually edited climbs.\n\n"
            "Re-detecting climbs will overwrite all existing climbs,\n"
            "including manual changes.\n\nContinue?",
            QMessageBox::Cancel | QMessageBox::Yes,
            QMessageBox::Cancel);
        if (answer != QMessageBox::Yes)
            return;
    }

    repo.deleteClimbsForActivity(m_controller->currentActivityId());

    ClimbDetector::Config cfg;
    cfg.minLengthKm          = m_climbMinLengthSpin    ? m_climbMinLengthSpin->value()    : 0.15;
    cfg.minElevationGainM    = m_climbMinGainSpin      ? m_climbMinGainSpin->value()      : 10.0;
    cfg.minAverageGradient   = m_climbMinGradientSpin  ? m_climbMinGradientSpin->value()  : 4.0;
    cfg.startGradient        = m_climbStartGradientSpin? m_climbStartGradientSpin->value(): 1.5;
    cfg.maxDipMeters         = m_climbDipMetersSpin    ? m_climbDipMetersSpin->value()    : 10.0;
    cfg.maxDipDistanceMeters = m_climbDipDistanceSpin  ? m_climbDipDistanceSpin->value()  : 200.0;
    cfg.elevationSmoothingMeters = m_climbSmoothingSpin? m_climbSmoothingSpin->value()    : 50.0;

    m_detectedClimbs = ClimbDetector::detect(ride, cfg);
    applyWeightMetricsToClimbs(m_detectedClimbs, resolveActiveActivityWeightKg());

    for (Climb& c : m_detectedClimbs)
    {
        ClimbRecord record;
        record.activityId      = m_controller->currentActivityId();
        record.startSeconds    = c.startSeconds;
        record.endSeconds      = c.endSeconds;
        record.name            = c.name;
        record.elevationGainM  = c.elevationGainM;
        record.lengthKm        = c.lengthKm;
        record.averageGradient = c.averageGradient;
        record.source          = QStringLiteral("auto");
        record.locked          = false;
        c.id = repo.insertClimb(record);
    }

    refreshClimbViews();
    statusBar()->showMessage(
        QString("Detected %1 climb(s) and saved to database.").arg(m_detectedClimbs.size()),
        3000);
}

void MainWindow::triggerDetectIntervals()
{
    if (!m_dbManager.isOpen() || m_controller->currentActivityId() <= 0)
    {
        QMessageBox::information(this, "Detect Intervals", "No activity loaded.");
        return;
    }

    auto db = m_dbManager.database();
    IntervalRepository repo(db);

    if (repo.hasLockedIntervals(m_controller->currentActivityId()))
    {
        const int answer = QMessageBox::warning(
            this,
            "Detect Intervals",
            "This activity contains manually edited intervals.\n\n"
            "Re-detecting intervals will overwrite all existing intervals,\n"
            "including manual changes.\n\nContinue?",
            QMessageBox::Cancel | QMessageBox::Yes,
            QMessageBox::Cancel);
        if (answer != QMessageBox::Yes)
            return;
    }

    repo.deleteIntervalsForActivity(m_controller->currentActivityId());

    IntervalDetector::Config cfg;
    cfg.ftp = m_controller->ftp();
    const std::vector<Interval> detected = IntervalDetector::detect(m_controller->rideData(), cfg);

    const auto deduplicated = IntervalDetector::removeOverlappingAndDuplicates(detected);

    for (const Interval& iv : deduplicated)
    {
        IntervalRecord record;
        record.activityId  = m_controller->currentActivityId();
        record.startSample = static_cast<int>(std::round(iv.startSeconds));
        record.endSample   = static_cast<int>(std::round(iv.endSeconds));
        record.name        = QString::fromStdString(iv.label);
        record.type        = QString::fromStdString(iv.label);
        record.avgPower    = iv.averagePower;
        record.np          = iv.normalizedPower;
        record.avgHr       = iv.averageHeartRate;
        record.avgCadence  = iv.averageCadence;
        record.source      = QStringLiteral("auto");
        record.locked      = false;
        repo.insertInterval(record);
    }

    updateIntervals();
    statusBar()->showMessage(
        QString("Detected %1 interval(s) (after deduplication) and saved to database.").arg(deduplicated.size()),
        3000);
}

void MainWindow::editCurrentActivityProperties()
{
    if (!m_dbManager.isOpen() || m_controller->currentActivityId() <= 0)
        return;

    auto db = m_dbManager.database();
    ActivityRepository repo(db);
    Activity activity = repo.getActivity(m_controller->currentActivityId());
    if (!activity.isValid())
        return;

    EditActivityDialog dlg(activity, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    const Activity updated = dlg.updatedActivity();
    if (!repo.updateActivityProperties(activity.id, updated))
    {
        QMessageBox::critical(this, "Error", "Failed to save activity properties.");
        return;
    }

    statusBar()->showMessage("Activity properties updated.", 2500);
}

void MainWindow::showAbout()
{
    AboutDialog dlg(this);
    dlg.exec();
}
