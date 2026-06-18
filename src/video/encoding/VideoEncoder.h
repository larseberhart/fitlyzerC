// SPDX-License-Identifier: GPL-3

#pragma once

#include <QString>
#include <QStringList>

class QProcess;
struct VideoExportSettings;

namespace VideoEncoder
{
QStringList ffmpegArguments(const VideoExportSettings& settings, int fps);
bool startFfmpeg(QProcess& process,
                 const QString& ffmpegPath,
                 const VideoExportSettings& settings,
                 int fps,
                 QString& errorMessage);
}
