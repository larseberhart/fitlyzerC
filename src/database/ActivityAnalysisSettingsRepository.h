#pragma once

#include <QList>
#include <QString>

class QSqlDatabase;

struct ActivityAnalysisSettings
{
    int    activityId             = -1;
    double climbMinGain           = 30.0;
    double climbMinLength         = 0.5;
    double climbMinGradient       = 3.0;
    double intervalWorkThreshold  = 0.0;
    double intervalRestThreshold  = 0.0;

    bool isValid() const { return activityId > 0; }
};

class ActivityAnalysisSettingsRepository
{
public:
    explicit ActivityAnalysisSettingsRepository(QSqlDatabase& db);

    bool insertOrUpdate(const ActivityAnalysisSettings& settings);
    ActivityAnalysisSettings settingsForActivity(int activityId);

private:
    QSqlDatabase& m_db;
};
