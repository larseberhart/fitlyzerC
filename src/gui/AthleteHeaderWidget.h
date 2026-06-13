#pragma once

#include <QWidget>

class QLabel;

class AthleteHeaderWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AthleteHeaderWidget(QWidget* parent = nullptr);

    void setSummary(const QString& athleteName,
                    int ftpWatts,
                    int activityCount,
                    const QString& lastActivityDate);

private:
    QLabel* m_titleLabel = nullptr;
    QLabel* m_nameLabel = nullptr;
    QLabel* m_metricsLabel = nullptr;
};
