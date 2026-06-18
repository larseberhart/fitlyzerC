// SPDX-License-Identifier: GPL-3

#include "VideoEncoder.h"

#include <QProcess>

#include "video/VideoExportSettings.h"

namespace VideoEncoder
{
QStringList ffmpegArguments(const VideoExportSettings& settings, int fps)
{
    return {
        "-y",
        "-f", "rawvideo",
        "-pixel_format", "rgba",
        "-video_size", QString("%1x%2").arg(settings.width).arg(settings.height),
        "-framerate", QString::number(fps),
        "-i", "-",
        "-an",
        "-c:v", "libx264",
        "-preset", "medium",
        "-crf", "20",
        "-pix_fmt", "yuv420p",
        settings.outputPath
    };
}

bool startFfmpeg(QProcess& process,
                 const QString& ffmpegPath,
                 const VideoExportSettings& settings,
                 int fps,
                 QString& errorMessage)
{
    process.setProgram(ffmpegPath);
    process.setArguments(ffmpegArguments(settings, fps));
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start();

    if (process.waitForStarted())
        return true;

    const QString detail = process.errorString().trimmed();
    if (detail.isEmpty())
        errorMessage = QString("Failed to start FFmpeg at '%1'.").arg(ffmpegPath);
    else
        errorMessage = QString("Failed to start FFmpeg at '%1': %2").arg(ffmpegPath, detail);
    return false;
}
}
