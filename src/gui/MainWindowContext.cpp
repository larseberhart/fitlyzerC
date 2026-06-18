// SPDX-License-Identifier: GPL-3

#include "MainWindow.h"

#include "database/ActivityRepository.h"
#include "database/AthleteRepository.h"
#include "fit/RideData.h"

#include <algorithm>
#include <limits>

void MainWindow::refreshAthleteSelector()
{
    if (!m_athleteCombo)
        return;

    const QSignalBlocker blocker(m_athleteCombo);
    m_athleteCombo->clear();
    m_athleteCombo->addItem("Select athlete...", -1);

    if (m_dbManager.isOpen())
    {
        auto db = m_dbManager.database();
        AthleteRepository repo(db);
        for (const Athlete& athlete : repo.listAthletes())
            m_athleteCombo->addItem(athlete.fullName(), athlete.id);
    }

    const int idx = m_athleteCombo->findData(m_currentAthleteId);
    if (idx >= 0)
    {
        m_athleteCombo->setCurrentIndex(idx);
    }
    else
    {
        m_athleteCombo->setCurrentIndex(0);
        m_currentAthleteId = -1;
        m_currentAthleteName.clear();
        m_controller->setCurrentAthlete(-1);
    }
}

ColorMetric MainWindow::currentColorMetric() const
{
    if (!m_colorMetricCombo)
        return ColorMetric::None;

    return static_cast<ColorMetric>(m_colorMetricCombo->currentData().toInt());
}

ColorContext MainWindow::buildColorContext() const
{
    ColorContext context;
    context.ftp = static_cast<int>(m_controller->ftp());

    if (m_dbManager.isOpen() && m_currentAthleteId > 0)
    {
        auto db = m_dbManager.database();
        AthleteRepository repo(db);

        int resolvedFtp = 0;
        if (m_controller->currentActivityId() > 0)
        {
            ActivityRepository actRepo(db);
            const Activity activity = actRepo.getActivity(m_controller->currentActivityId());
            const QString rawDate = !activity.startTime.isEmpty()
                ? activity.startTime.left(10)
                : activity.importedAt.left(10);
            const QDate ftpDate = QDate::fromString(rawDate, Qt::ISODate);
            if (ftpDate.isValid())
                resolvedFtp = repo.getFtpForDate(m_currentAthleteId, ftpDate);
        }

        if (resolvedFtp <= 0)
            resolvedFtp = repo.getAthlete(m_currentAthleteId).ftpWatts;

        if (resolvedFtp > 0)
            context.ftp = resolvedFtp;
    }

    int maxHeartRate = 0;
    double minAltitude = std::numeric_limits<double>::max();
    double maxAltitude = std::numeric_limits<double>::lowest();
    bool hasAltitude = false;

    for (const RideRecord& record : m_controller->rideData().records)
    {
        if (record.hasHeartRate)
            maxHeartRate = std::max(maxHeartRate, static_cast<int>(record.heartRate));
        if (record.hasAltitude)
        {
            minAltitude = std::min(minAltitude, record.altitude);
            maxAltitude = std::max(maxAltitude, record.altitude);
            hasAltitude = true;
        }
    }

    if (maxHeartRate > 0)
        context.heartRateMax = maxHeartRate;
    if (hasAltitude)
    {
        context.altitudeMin = minAltitude;
        context.altitudeMax = maxAltitude;
        context.hasAltitudeRange = true;
    }

    return context;
}
