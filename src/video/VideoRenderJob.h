#pragma once

#include <QAtomicInteger>
#include <QObject>

#include "video/VideoExportSettings.h"

class VideoRenderJob : public QObject
{
    Q_OBJECT

public:
    explicit VideoRenderJob(VideoExportSettings settings, QObject* parent = nullptr);

public slots:
    void run();
    void cancel();

signals:
    void stageChanged(const QString& stage);
    void progressChanged(int value, int maximum);
    void frameProgressChanged(int currentFrame, int totalFrames, const QString& etaText);
    void tileStatsChanged(const QString& statsText);
    void finished(bool success, const QString& message, bool canceled);

private:
    VideoExportSettings m_settings;
    QAtomicInteger<int> m_cancelRequested { 0 };
};
