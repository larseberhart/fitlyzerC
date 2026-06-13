#include "AthleteHeaderWidget.h"

#include <QLabel>
#include <QVBoxLayout>

AthleteHeaderWidget::AthleteHeaderWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(4);

    m_titleLabel = new QLabel("Current Athlete", this);
    m_nameLabel = new QLabel("No athlete selected", this);
    m_metricsLabel = new QLabel("FTP: 0 W   Activities: 0   Last Activity: -", this);

    m_nameLabel->setStyleSheet("font-size: 16px; font-weight: 600;");
    setStyleSheet("AthleteHeaderWidget { background: #f8fafc; border: 1px solid #d6dde8; border-radius: 6px; }");

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_nameLabel);
    layout->addWidget(m_metricsLabel);
}

void AthleteHeaderWidget::setSummary(const QString& athleteName,
                                     int ftpWatts,
                                     int activityCount,
                                     const QString& lastActivityDate)
{
    const QString displayName = athleteName.isEmpty() ? QString("No athlete selected") : athleteName;
    m_nameLabel->setText(displayName);
    m_metricsLabel->setText(
        QString("FTP: %1 W   Activities: %2   Last Activity: %3")
            .arg(ftpWatts)
            .arg(activityCount)
            .arg(lastActivityDate));
}
