// SPDX-License-Identifier: GPL-3

#include "MainWindow.h"

#include <QDate>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

#include "charts/RideChartWidget.h"
#include "core/settings/DateFormatter.h"
#include "database/ActivityRepository.h"
#include "database/AthleteRepository.h"
#include "maps/MapRenderer.h"
#include "platform/Platform.h"
#include "utils/FfmpegPath.h"
#include "video/VideoExportDialog.h"
#include "video/VideoExportSettings.h"

#include <algorithm>

namespace
{
QString sanitizeVideoFilePart(QString text)
{
    static const QString invalid = QStringLiteral("\\/:*?\"<>|");
    for (const QChar c : invalid)
        text.replace(c, '-');
    text = text.simplified().replace(' ', '-');
    while (text.contains("--"))
        text.replace("--", "-");
    return text.trimmed();
}
} // namespace

void MainWindow::createVideo()
{
    const RideData& rideData = m_controller->rideData();
    if (rideData.records.empty())
    {
        QMessageBox::information(this, "Create Video", "Load an activity before creating a video.");
        return;
    }

    if (m_controller->currentActivityId() <= 0 || !m_dbManager.isOpen())
    {
        QMessageBox::information(this, "Create Video", "Video export is available for saved activities.");
        return;
    }

    auto db = m_dbManager.database();
    ActivityRepository actRepo(db);
    const Activity activity = actRepo.getActivity(m_controller->currentActivityId());

    QString activityName = QFileInfo(activity.fileName).completeBaseName();
    if (activityName.isEmpty())
        activityName = activity.sport;
    if (activityName.isEmpty())
        activityName = QStringLiteral("Ride");

    QString athleteName = m_currentAthleteName.trimmed();
    if (athleteName.isEmpty() && m_currentAthleteId > 0)
    {
        AthleteRepository athleteRepo(db);
        athleteName = athleteRepo.getAthlete(m_currentAthleteId).fullName().trimmed();
    }
    if (athleteName.isEmpty())
        athleteName = QStringLiteral("Athlete");

    const QDate date = !activity.startTime.isEmpty()
        ? QDate::fromString(activity.startTime.left(10), Qt::ISODate)
        : QDate::fromString(activity.importedAt.left(10), Qt::ISODate);
    const QString dateText = DateFormatter::toIsoDate(date.isValid() ? date : QDate::currentDate());

    const QString defaultName = QString("%1-%2-%3.mp4")
        .arg(dateText,
             sanitizeVideoFilePart(activityName),
             sanitizeVideoFilePart(athleteName));
    const QString defaultOutput = QDir(Platform::defaultDatabaseDirectory()).filePath(defaultName);

    const double visibleStartMin = m_powerChart ? m_powerChart->visibleRangeStartMinutes() : 0.0;
    const double visibleEndMin = m_powerChart ? m_powerChart->visibleRangeEndMinutes() : 0.0;

    VideoExportSettings defaults;
    defaults.outputPath = defaultOutput;
    defaults.ffmpegPath = resolveFfmpegExecutablePath();
    defaults.width = 1920;
    defaults.height = 1080;
    defaults.fps = 30;
    defaults.playbackSpeed = 3.0;
    defaults.useSelectedSegment = true;
    defaults.segmentStartSeconds = std::max(0.0, std::min(visibleStartMin, visibleEndMin) * 60.0);
    defaults.segmentEndSeconds = std::max(defaults.segmentStartSeconds,
                                          std::max(visibleStartMin, visibleEndMin) * 60.0);
    defaults.mapStyle = m_mapRenderer ? m_mapRenderer->mapStyle() : MapStyle::Light;
    defaults.autoZoom = true;
    defaults.fixedZoom = 18;
    defaults.followAthlete = true;
    defaults.routeColorMetric = currentColorMetric() == ColorMetric::None
        ? ColorMetric::Power
        : currentColorMetric();
    defaults.athleteName = athleteName;
    defaults.activityName = activityName;
    defaults.colorContext = buildColorContext();
    defaults.rideData = rideData;

    if (defaults.segmentEndSeconds - defaults.segmentStartSeconds < 1.0)
    {
        defaults.segmentStartSeconds = 0.0;
        defaults.segmentEndSeconds = rideData.records.back().elapsedSeconds;
    }

    VideoExportDialog dialog(defaults, this);
    dialog.exec();
}
