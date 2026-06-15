#pragma once

#include <QByteArray>
#include <QString>

class QSqlDatabase;

/// Simple blob cache for expensive per-activity analysis results
/// (power curve, histogram, best efforts, etc.).
class ActivityAnalysisCacheRepository
{
public:
    explicit ActivityAnalysisCacheRepository(QSqlDatabase& db);

    bool       setCache(int activityId, const QString& key, const QByteArray& value);
    QByteArray getCache(int activityId, const QString& key);
    bool       hasCache(int activityId, const QString& key);
    bool       invalidateCache(int activityId);
    bool       invalidateKey(int activityId, const QString& key);

private:
    QSqlDatabase& m_db;
};
