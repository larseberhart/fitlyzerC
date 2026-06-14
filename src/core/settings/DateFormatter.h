#pragma once

#include <QDate>
#include <QDateTime>
#include <QString>

class DateFormatter
{
public:
    static QString formatDate(const QDate& date);
    static QString formatDateTime(const QDateTime& dateTime);
    static QString toIsoDate(const QDate& date);
    static QString toIsoDateTime(const QDateTime& dateTime);

    static QDate parseDate(const QString& text);
    static QString qtDatePattern();
};
