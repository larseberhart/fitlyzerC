#pragma once

#include <QList>
#include <QString>

class QSqlDatabase;

struct ImportRecord
{
    int     id           = -1;
    QString fileName;
    QString importedAt;
    QString status;       // "success", "duplicate", "error"
    int     activityId   = -1;
    QString errorMessage;
};

class ImportRepository
{
public:
    explicit ImportRepository(QSqlDatabase& db);

    int              logImport(const ImportRecord& r);
    QList<ImportRecord> getRecentImports(int limit = 50);

private:
    QSqlDatabase& m_db;
};
