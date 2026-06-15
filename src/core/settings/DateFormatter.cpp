// SPDX-License-Identifier: GPL-3

/**
 * @file DateFormatter.cpp
 * @brief Application settings support for DateFormatter.
 *
 * Defines settings and formatting helpers used to configure application behavior and presentation.
 *
 * Responsibilities:
 * - Provide settings persistence or settings-related utilities
 *
 * @author Lars EBERHART
 */

#include "DateFormatter.h"

#include "AppSettings.h"

#include <QStringList>

namespace
{
QString datePatternForFormat(DateFormat format)
{
    switch (format)
    {
        case DateFormat::DD_MM_YYYY:
            return QStringLiteral("dd-MM-yyyy");
        case DateFormat::YYYY_MM_DD:
            return QStringLiteral("yyyy-MM-dd");
        case DateFormat::DD_DOT_MM_DOT_YYYY:
            return QStringLiteral("dd.MM.yyyy");
        case DateFormat::MM_DD_YYYY:
            return QStringLiteral("MM/dd/yyyy");
    }

    return QStringLiteral("dd-MM-yyyy");
}
}

QString DateFormatter::formatDate(const QDate& date)
{
    if (!date.isValid())
        return QStringLiteral("-");

    return date.toString(datePatternForFormat(AppSettings::instance().dateFormat()));
}

QString DateFormatter::formatDateTime(const QDateTime& dateTime)
{
    if (!dateTime.isValid())
        return QStringLiteral("-");

    return QStringLiteral("%1 %2")
        .arg(formatDate(dateTime.date()), dateTime.time().toString(QStringLiteral("HH:mm")));
}

QString DateFormatter::toIsoDate(const QDate& date)
{
    return date.toString(Qt::ISODate);
}

QString DateFormatter::toIsoDateTime(const QDateTime& dateTime)
{
    return dateTime.toString(Qt::ISODate);
}

QDate DateFormatter::parseDate(const QString& text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return QDate();

    const QString activePattern = datePatternForFormat(AppSettings::instance().dateFormat());

    QDate parsed = QDate::fromString(trimmed, activePattern);
    if (parsed.isValid())
        return parsed;

    parsed = QDate::fromString(trimmed, Qt::ISODate);
    if (parsed.isValid())
        return parsed;

    // Accept dates that may have been entered using any supported format.
    static const QStringList allPatterns = {
        QStringLiteral("dd-MM-yyyy"),
        QStringLiteral("yyyy-MM-dd"),
        QStringLiteral("dd.MM.yyyy"),
        QStringLiteral("MM/dd/yyyy")
    };

    for (const QString& pattern : allPatterns)
    {
        parsed = QDate::fromString(trimmed, pattern);
        if (parsed.isValid())
            return parsed;
    }

    return QDate();
}

QString DateFormatter::qtDatePattern()
{
    return datePatternForFormat(AppSettings::instance().dateFormat());
}