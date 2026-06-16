// SPDX-License-Identifier: GPL-3


#pragma once

#include <QAtomicInteger>
#include <QObject>

#include "video/VideoExportSettings.h"

/**
 * @brief Renders activity video with map overlay and data visualization.
 *
 * Generates video file from activity GPS and performance data,
 * handling frame rendering, ffmpeg encoding, and progress reporting.
 */
class VideoRenderJob : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a video render job.
     * @param settings Export settings.
     * @param parent Parent object.
     */
    explicit VideoRenderJob(VideoExportSettings settings, QObject* parent = nullptr);

    /**
     * @slot Runs the video rendering process.
     */
    void run();

    /**
     * @slot Requests cancellation of rendering.
     */
    void cancel();

signals:
    /// @signal Emitted when render stage changes.
    /// @param stage Description of current stage.
    void stageChanged(const QString& stage);

    /// @signal Emitted when progress updates.
    /// @param value Current progress value.
    /// @param maximum Maximum progress value.
    void progressChanged(int value, int maximum);

    /// @signal Emitted with frame-level progress.
    /// @param currentFrame Current frame number.
    /// @param totalFrames Total number of frames.
    /// @param etaText Estimated time remaining.
    void frameProgressChanged(int currentFrame, int totalFrames, const QString& etaText);

    /// @signal Emitted when tile cache statistics update.
    /// @param statsText Formatted statistics text.
    void tileStatsChanged(const QString& statsText);

    /// @signal Emitted when rendering completes or fails.
    /// @param success True if render completed successfully.
    /// @param message Status or error message.
    /// @param canceled True if render was canceled by user.
    void finished(bool success, const QString& message, bool canceled);

private:
    VideoExportSettings m_settings;
    QAtomicInteger<int> m_cancelRequested { 0 };
};