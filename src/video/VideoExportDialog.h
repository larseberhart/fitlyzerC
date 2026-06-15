// SPDX-License-Identifier: GPL-3

/**
 * @file VideoExportDialog.h
 * @brief Video export and rendering component for VideoExportDialog.
 *
 * Implements video rendering, tile provision, and export settings used for activity video generation.
 *
 * Responsibilities:
 * - Provide video rendering or export-related functionality
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QDialog>

#include "video/VideoExportSettings.h"

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QThread;

class VideoRenderJob;

/**
 * @brief Dialog for configuring and launching activity video export.
 *
 * Provides UI for selecting output file, video format, map style, data overlays,
 * and other video generation settings, then launches rendering job.
 */
class VideoExportDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructs video export dialog.
     * @param defaults Default export settings.
     * @param parent Parent widget.
     */
    explicit VideoExportDialog(const VideoExportSettings& defaults, QWidget* parent = nullptr);
    ~VideoExportDialog() override;

private slots:
    void chooseOutputFile();
    void startExport();
    void cancelOrClose();
    void onStageChanged(const QString& stage);
    void onProgressChanged(int value, int maximum);
    void onFrameProgressChanged(int currentFrame, int totalFrames, const QString& etaText);
    void onTileStatsChanged(const QString& statsText);
    void onFinished(bool success, const QString& message, bool canceled);

private:
    VideoExportSettings collectSettings() const;
    void setAllOverlayChecks(bool enabled);
    void updateVideoLengthLabel();

    VideoExportSettings m_defaults;

    QLineEdit* m_outputEdit = nullptr;
    QComboBox* m_resolutionCombo = nullptr;
    QSpinBox* m_fpsSpin = nullptr;
    QDoubleSpinBox* m_speedSpin = nullptr;
    QLabel* m_videoLengthLabel = nullptr;

    QRadioButton* m_selectedSegmentRadio = nullptr;
    QRadioButton* m_entireActivityRadio = nullptr;

    QComboBox* m_mapStyleCombo = nullptr;
    QSpinBox* m_fixedZoomSpin = nullptr;

    QComboBox* m_routeColorCombo = nullptr;

    QCheckBox* m_powerCheck = nullptr;
    QCheckBox* m_hrCheck = nullptr;
    QCheckBox* m_cadenceCheck = nullptr;
    QCheckBox* m_speedCheck = nullptr;
    QCheckBox* m_altitudeCheck = nullptr;
    QCheckBox* m_timeCheck = nullptr;
    QCheckBox* m_athleteCheck = nullptr;
    QCheckBox* m_telemetryPanelCheck = nullptr;
    QCheckBox* m_legendCheck = nullptr;
    QCheckBox* m_chartsCheck = nullptr;
    QCheckBox* m_deleteTemporaryTilesCheck = nullptr;

    QLabel* m_stageLabel = nullptr;
    QLabel* m_frameLabel = nullptr;
    QLabel* m_etaLabel = nullptr;
    QLabel* m_tileStatsLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;

    QPushButton* m_exportButton = nullptr;
    QPushButton* m_cancelButton = nullptr;

    QThread* m_thread = nullptr;
    VideoRenderJob* m_job = nullptr;
    bool m_running = false;
};