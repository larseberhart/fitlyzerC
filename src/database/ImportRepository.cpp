#include "ImportRepository.h"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>

ImportRepository::ImportRepository(QSqlDatabase& db)
    : m_db(db)
{}

int ImportRepository::logImport(const ImportRecord& r)
{
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO imports(file_name, imported_at, status, activity_id, error_message)"
        " VALUES(:fn, :ts, :st, :aid, :err)");
    q.bindValue(":fn",  r.fileName);
    q.bindValue(":ts",  r.importedAt.isEmpty()
                        ? QDateTime::currentDateTimeUtc().toString(Qt::ISODate)
                        : r.importedAt);
    q.bindValue(":st",  r.status);
    q.bindValue(":aid", r.activityId > 0 ? QVariant(r.activityId) : QVariant());
    q.bindValue(":err", r.errorMessage.isEmpty() ? QVariant() : QVariant(r.errorMessage));
    if (!q.exec()) return -1;
    return q.lastInsertId().toInt();
}

QList<ImportRecord> ImportRepository::getRecentImports(int limit)
{
    QList<ImportRecord> result;
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT id, file_name, imported_at, status, activity_id, error_message"
        " FROM imports ORDER BY imported_at DESC LIMIT :lim");
    q.bindValue(":lim", limit);
    q.exec();
    while (q.next())
    {
        ImportRecord r;
        r.id           = q.value(0).toInt();
        r.fileName     = q.value(1).toString();
        r.importedAt   = q.value(2).toString();
        r.status       = q.value(3).toString();
        r.activityId   = q.value(4).toInt();
        r.errorMessage = q.value(5).toString();
        result.append(r);
    }
    return result;
}
