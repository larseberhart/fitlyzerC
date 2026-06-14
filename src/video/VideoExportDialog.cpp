#include "VideoExportDialog.h"

#include "core/settings/DateFormatter.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QThread>
#include <QVBoxLayout>

#include "video/VideoRenderJob.h"

namespace
{
QString mapStyleName(MapStyle style)
{
    switch (style)
    {
        case MapStyle::Street: return "Street";
        case MapStyle::Light: return "Light";
        case MapStyle::Dark: return "Dark";
        case MapStyle::Satellite: return "Satellite";
        case MapStyle::Terrain: return "Terrain";
        case MapStyle::Topographic: return "Topographic";
        case MapStyle::Minimal: return "Minimal";
    }
    return "Street";
}

QString makeDefaultFilename(const VideoExportSettings& defaults)
{
    QString activity = defaults.activityName.trimmed();
    if (activity.isEmpty())
        activity = "Ride";

    QString athlete = defaults.athleteName.trimmed();
    if (athlete.isEmpty())
        athlete = "Athlete";

    const QString date = DateFormatter::toIsoDate(QDate::currentDate());
    QString name = QString("%1-%2-%3.mp4").arg(date, activity, athlete);

    static const QString invalid = QStringLiteral("\\/:*?\"<>|");
    for (const QChar c : invalid)
        name.replace(c, '-');

    while (name.contains("--"))
        name.replace("--", "-");

    return name;
}

QString formatVideoLength(double seconds)
{
    const int total = static_cast<int>(std::round(std::max(0.0, seconds)));
    const int h = total / 3600;
    const int m = (total % 3600) / 60;
    const int s = total % 60;
    if (h > 0)
        return QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
    return QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
}
}

VideoExportDialog::VideoExportDialog(const VideoExportSettings& defaults, QWidget* parent)
    : QDialog(parent)
    , m_defaults(defaults)
{
    setWindowTitle("Create Video");
    setModal(true);
    resize(620, 720);

    auto* root = new QVBoxLayout(this);

    auto* outputGroup = new QGroupBox("General", this);
    auto* outputLayout = new QGridLayout(outputGroup);
    m_outputEdit = new QLineEdit(outputGroup);
    if (defaults.outputPath.isEmpty())
        m_outputEdit->setText(makeDefaultFilename(defaults));
    else
        m_outputEdit->setText(defaults.outputPath);

    auto* browseButton = new QPushButton("Browse...", outputGroup);
    connect(browseButton, &QPushButton::clicked, this, &VideoExportDialog::chooseOutputFile);

    outputLayout->addWidget(new QLabel("Output file:"), 0, 0);
    outputLayout->addWidget(m_outputEdit, 0, 1);
    outputLayout->addWidget(browseButton, 0, 2);

    m_selectedSegmentRadio = new QRadioButton("Current selected segment", outputGroup);
    m_entireActivityRadio = new QRadioButton("Entire activity", outputGroup);
    m_selectedSegmentRadio->setChecked(defaults.useSelectedSegment);
    m_entireActivityRadio->setChecked(!defaults.useSelectedSegment);
    outputLayout->addWidget(m_selectedSegmentRadio, 1, 0, 1, 3);
    outputLayout->addWidget(m_entireActivityRadio, 2, 0, 1, 3);

    auto* videoGroup = new QGroupBox("Video", this);
    auto* videoLayout = new QFormLayout(videoGroup);

    m_resolutionCombo = new QComboBox(videoGroup);
    m_resolutionCombo->addItem("1280x720", QSize(1280, 720));
    m_resolutionCombo->addItem("1920x1080", QSize(1920, 1080));
    m_resolutionCombo->addItem("2560x1440", QSize(2560, 1440));
    m_resolutionCombo->addItem("3840x2160", QSize(3840, 2160));

    const int defaultResIndex = m_resolutionCombo->findData(QSize(defaults.width, defaults.height));
    m_resolutionCombo->setCurrentIndex(defaultResIndex >= 0 ? defaultResIndex : 1);

    m_fpsSpin = new QSpinBox(videoGroup);
    m_fpsSpin->setRange(15, 60);
    m_fpsSpin->setValue(defaults.fps <= 0 ? 30 : defaults.fps);

    m_speedSpin = new QDoubleSpinBox(videoGroup);
    m_speedSpin->setRange(1.0, 20.0);
    m_speedSpin->setSingleStep(0.5);
    m_speedSpin->setValue(defaults.playbackSpeed < 1.0 ? 3.0 : defaults.playbackSpeed);
    m_speedSpin->setSuffix("x");
    m_speedSpin->setToolTip("Acceleration factor: 1x = real-time, 5x = 5 seconds FIT per 1 second video.");

    videoLayout->addRow("Resolution:", m_resolutionCombo);
    videoLayout->addRow("FPS:", m_fpsSpin);
    videoLayout->addRow("Playback speed:", m_speedSpin);
    m_videoLengthLabel = new QLabel(videoGroup);
    videoLayout->addRow("Video Length:", m_videoLengthLabel);
    videoLayout->addRow("Format:", new QLabel("MP4 (H.264)", videoGroup));

    connect(m_selectedSegmentRadio, &QRadioButton::toggled, this, &VideoExportDialog::updateVideoLengthLabel);
    connect(m_entireActivityRadio, &QRadioButton::toggled, this, &VideoExportDialog::updateVideoLengthLabel);
    connect(m_speedSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &VideoExportDialog::updateVideoLengthLabel);

    auto* mapGroup = new QGroupBox("Map", this);
    auto* mapLayout = new QGridLayout(mapGroup);

    m_mapStyleCombo = new QComboBox(mapGroup);
    m_mapStyleCombo->addItem(mapStyleName(MapStyle::Street), static_cast<int>(MapStyle::Street));
    m_mapStyleCombo->addItem(mapStyleName(MapStyle::Light), static_cast<int>(MapStyle::Light));
    m_mapStyleCombo->addItem(mapStyleName(MapStyle::Dark), static_cast<int>(MapStyle::Dark));
    m_mapStyleCombo->addItem(mapStyleName(MapStyle::Satellite), static_cast<int>(MapStyle::Satellite));
    m_mapStyleCombo->addItem(mapStyleName(MapStyle::Terrain), static_cast<int>(MapStyle::Terrain));
    m_mapStyleCombo->addItem(mapStyleName(MapStyle::Topographic), static_cast<int>(MapStyle::Topographic));
    m_mapStyleCombo->addItem(mapStyleName(MapStyle::Minimal), static_cast<int>(MapStyle::Minimal));
    const int styleIndex = m_mapStyleCombo->findData(static_cast<int>(defaults.mapStyle));
    m_mapStyleCombo->setCurrentIndex(styleIndex >= 0 ? styleIndex : 0);

    m_fixedZoomSpin = new QSpinBox(mapGroup);
    m_fixedZoomSpin->setRange(1, 18);
    m_fixedZoomSpin->setValue(defaults.fixedZoom <= 0 ? 18 : defaults.fixedZoom);

    mapLayout->addWidget(new QLabel("Style:"), 0, 0);
    mapLayout->addWidget(m_mapStyleCombo, 0, 1, 1, 2);
    mapLayout->addWidget(new QLabel("Zoom:"), 1, 0);
    mapLayout->addWidget(m_fixedZoomSpin, 1, 1);
    auto* followLabel = new QLabel("Follow athlete: enabled", mapGroup);
    mapLayout->addWidget(followLabel, 2, 0, 1, 3);

    auto* routeGroup = new QGroupBox("Route", this);
    auto* routeLayout = new QFormLayout(routeGroup);
    m_routeColorCombo = new QComboBox(routeGroup);
    m_routeColorCombo->addItem("FTP Zone", static_cast<int>(ColorMetric::Power));
    m_routeColorCombo->addItem("Power", static_cast<int>(ColorMetric::Power));
    m_routeColorCombo->addItem("Heart Rate", static_cast<int>(ColorMetric::HeartRate));
    m_routeColorCombo->addItem("Cadence", static_cast<int>(ColorMetric::Cadence));
    m_routeColorCombo->addItem("Speed", static_cast<int>(ColorMetric::Speed));
    m_routeColorCombo->addItem("Altitude", static_cast<int>(ColorMetric::Altitude));
    m_routeColorCombo->addItem("Single Color", static_cast<int>(ColorMetric::None));
    routeLayout->addRow("Color route by:", m_routeColorCombo);

    auto* telemetryGroup = new QGroupBox("Telemetry Overlays", this);
    auto* telemetryLayout = new QGridLayout(telemetryGroup);

    m_powerCheck = new QCheckBox("Power", telemetryGroup);
    m_hrCheck = new QCheckBox("Heart Rate", telemetryGroup);
    m_cadenceCheck = new QCheckBox("Cadence", telemetryGroup);
    m_speedCheck = new QCheckBox("Speed", telemetryGroup);
    m_altitudeCheck = new QCheckBox("Altitude", telemetryGroup);
    m_timeCheck = new QCheckBox("Time", telemetryGroup);
    m_athleteCheck = new QCheckBox("Athlete Name", telemetryGroup);
    m_telemetryPanelCheck = new QCheckBox("Telemetry Box", telemetryGroup);
    m_legendCheck = new QCheckBox("Route Legend", telemetryGroup);
    m_chartsCheck = new QCheckBox("Metric Charts", telemetryGroup);

    m_powerCheck->setChecked(defaults.overlayPower);
    m_hrCheck->setChecked(defaults.overlayHeartRate);
    m_cadenceCheck->setChecked(defaults.overlayCadence);
    m_speedCheck->setChecked(defaults.overlaySpeed);
    m_altitudeCheck->setChecked(defaults.overlayAltitude);
    m_timeCheck->setChecked(defaults.overlayTime);
    m_athleteCheck->setChecked(defaults.overlayAthleteName);
    m_telemetryPanelCheck->setChecked(defaults.overlayTelemetryPanel);
    m_legendCheck->setChecked(defaults.overlayRouteLegend);
    m_chartsCheck->setChecked(defaults.overlayCharts);

    telemetryLayout->addWidget(m_powerCheck, 0, 0);
    telemetryLayout->addWidget(m_hrCheck, 0, 1);
    telemetryLayout->addWidget(m_cadenceCheck, 0, 2);
    telemetryLayout->addWidget(m_speedCheck, 1, 0);
    telemetryLayout->addWidget(m_altitudeCheck, 1, 1);
    telemetryLayout->addWidget(m_timeCheck, 1, 2);
    telemetryLayout->addWidget(m_athleteCheck, 2, 0);
    telemetryLayout->addWidget(m_telemetryPanelCheck, 2, 1);
    telemetryLayout->addWidget(m_legendCheck, 2, 2);
    telemetryLayout->addWidget(m_chartsCheck, 3, 0);

    auto* overlayButtons = new QHBoxLayout;
    auto* enableAllOverlaysButton = new QPushButton("Enable All", telemetryGroup);
    auto* disableAllOverlaysButton = new QPushButton("Disable All", telemetryGroup);
    connect(enableAllOverlaysButton, &QPushButton::clicked, this, [this]() { setAllOverlayChecks(true); });
    connect(disableAllOverlaysButton, &QPushButton::clicked, this, [this]() { setAllOverlayChecks(false); });
    overlayButtons->addWidget(enableAllOverlaysButton);
    overlayButtons->addWidget(disableAllOverlaysButton);
    overlayButtons->addStretch(1);
    telemetryLayout->addLayout(overlayButtons, 4, 0, 1, 3);

    auto* progressGroup = new QGroupBox("Export Progress", this);
    auto* progressLayout = new QVBoxLayout(progressGroup);
    m_stageLabel = new QLabel("Ready", progressGroup);
    m_frameLabel = new QLabel("Frame: -", progressGroup);
    m_etaLabel = new QLabel("ETA: -", progressGroup);
    m_progressBar = new QProgressBar(progressGroup);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);

    progressLayout->addWidget(m_stageLabel);
    progressLayout->addWidget(m_frameLabel);
    progressLayout->addWidget(m_etaLabel);
    progressLayout->addWidget(m_progressBar);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch(1);
    m_exportButton = new QPushButton("Export", this);
    m_cancelButton = new QPushButton("Cancel", this);
    buttons->addWidget(m_exportButton);
    buttons->addWidget(m_cancelButton);

    connect(m_exportButton, &QPushButton::clicked, this, &VideoExportDialog::startExport);
    connect(m_cancelButton, &QPushButton::clicked, this, &VideoExportDialog::cancelOrClose);

    root->addWidget(outputGroup);
    root->addWidget(videoGroup);
    root->addWidget(mapGroup);
    root->addWidget(routeGroup);
    root->addWidget(telemetryGroup);
    root->addWidget(progressGroup);
    root->addLayout(buttons);

    updateVideoLengthLabel();
}

VideoExportDialog::~VideoExportDialog()
{
    if (m_job)
        m_job->cancel();
    if (m_thread)
    {
        m_thread->quit();
        m_thread->wait();
    }
}

void VideoExportDialog::chooseOutputFile()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        "Export Video",
        m_outputEdit->text().trimmed(),
        "MP4 Video (*.mp4)");
    if (!path.isEmpty())
        m_outputEdit->setText(path);
}

VideoExportSettings VideoExportDialog::collectSettings() const
{
    VideoExportSettings s = m_defaults;
    s.outputPath = m_outputEdit->text().trimmed();

    const QSize res = m_resolutionCombo->currentData().toSize();
    s.width = res.width();
    s.height = res.height();
    s.fps = m_fpsSpin->value();
    s.playbackSpeed = m_speedSpin->value();

    s.useSelectedSegment = m_selectedSegmentRadio->isChecked();

    s.mapStyle = static_cast<MapStyle>(m_mapStyleCombo->currentData().toInt());
    s.autoZoom = false;
    s.fixedZoom = m_fixedZoomSpin->value();

    s.routeColorMetric = static_cast<ColorMetric>(m_routeColorCombo->currentData().toInt());

    s.overlayPower = m_powerCheck->isChecked();
    s.overlayHeartRate = m_hrCheck->isChecked();
    s.overlayCadence = m_cadenceCheck->isChecked();
    s.overlaySpeed = m_speedCheck->isChecked();
    s.overlayAltitude = m_altitudeCheck->isChecked();
    s.overlayTime = m_timeCheck->isChecked();
    s.overlayAthleteName = m_athleteCheck->isChecked();
    s.overlayTelemetryPanel = m_telemetryPanelCheck->isChecked();
    s.overlayRouteLegend = m_legendCheck->isChecked();
    s.overlayCharts = m_chartsCheck->isChecked();

    return s;
}

void VideoExportDialog::startExport()
{
    if (m_running)
        return;

    VideoExportSettings settings = collectSettings();
    if (settings.outputPath.isEmpty())
    {
        QMessageBox::warning(this, "Create Video", "Choose an output file.");
        return;
    }

    if (!settings.outputPath.endsWith(".mp4", Qt::CaseInsensitive))
        settings.outputPath += ".mp4";

    m_thread = new QThread(this);
    m_job = new VideoRenderJob(settings);
    m_job->moveToThread(m_thread);

    connect(m_thread, &QThread::started, m_job, &VideoRenderJob::run);
    connect(m_job, &VideoRenderJob::stageChanged, this, &VideoExportDialog::onStageChanged);
    connect(m_job, &VideoRenderJob::progressChanged, this, &VideoExportDialog::onProgressChanged);
    connect(m_job, &VideoRenderJob::frameProgressChanged, this, &VideoExportDialog::onFrameProgressChanged);
    connect(m_job, &VideoRenderJob::finished, this, &VideoExportDialog::onFinished);
    connect(m_job, &VideoRenderJob::finished, m_thread, &QThread::quit);
    connect(m_thread, &QThread::finished, m_job, &QObject::deleteLater);
    connect(m_thread, &QThread::finished, m_thread, &QObject::deleteLater);

    m_running = true;
    m_exportButton->setEnabled(false);
    m_cancelButton->setText("Cancel Export");

    m_stageLabel->setText("Preparing data...");
    m_frameLabel->setText("Frame: 0 / 0");
    m_etaLabel->setText("ETA: --:--");
    m_progressBar->setValue(0);

    m_thread->start();
}

void VideoExportDialog::cancelOrClose()
{
    if (m_running && m_job)
    {
        m_job->cancel();
        m_cancelButton->setEnabled(false);
        return;
    }

    reject();
}

void VideoExportDialog::onStageChanged(const QString& stage)
{
    m_stageLabel->setText(stage);
}

void VideoExportDialog::onProgressChanged(int value, int maximum)
{
    m_progressBar->setRange(0, std::max(1, maximum));
    m_progressBar->setValue(value);
}

void VideoExportDialog::onFrameProgressChanged(int currentFrame, int totalFrames, const QString& etaText)
{
    m_frameLabel->setText(QString("Frame: %1 / %2").arg(currentFrame).arg(totalFrames));
    m_etaLabel->setText(QString("ETA: %1").arg(etaText));
}

void VideoExportDialog::onFinished(bool success, const QString& message, bool canceled)
{
    m_running = false;
    m_exportButton->setEnabled(true);
    m_cancelButton->setEnabled(true);
    m_cancelButton->setText("Close");

    m_thread = nullptr;
    m_job = nullptr;

    if (success)
    {
        QMessageBox::information(this, "Create Video", message);
        accept();
        return;
    }

    if (!canceled)
        QMessageBox::critical(this, "Create Video", message);
}

void VideoExportDialog::setAllOverlayChecks(bool enabled)
{
    m_powerCheck->setChecked(enabled);
    m_hrCheck->setChecked(enabled);
    m_cadenceCheck->setChecked(enabled);
    m_speedCheck->setChecked(enabled);
    m_altitudeCheck->setChecked(enabled);
    m_timeCheck->setChecked(enabled);
    m_athleteCheck->setChecked(enabled);
    m_telemetryPanelCheck->setChecked(enabled);
    m_legendCheck->setChecked(enabled);
    m_chartsCheck->setChecked(enabled);
}

void VideoExportDialog::updateVideoLengthLabel()
{
    double realSeconds = 0.0;
    if (m_selectedSegmentRadio->isChecked())
    {
        realSeconds = std::max(0.0, m_defaults.segmentEndSeconds - m_defaults.segmentStartSeconds);
    }
    else if (!m_defaults.rideData.records.empty())
    {
        realSeconds = std::max(0.0, m_defaults.rideData.records.back().elapsedSeconds);
    }

    const double speed = std::max(1.0, m_speedSpin->value());
    const double videoSeconds = realSeconds / speed;
    m_videoLengthLabel->setText(
        QString("Real %1 / Video %2")
            .arg(formatVideoLength(realSeconds), formatVideoLength(videoSeconds)));
}
