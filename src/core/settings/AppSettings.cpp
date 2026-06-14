#include "AppSettings.h"

#include <QSettings>

AppSettings& AppSettings::instance()
{
    static AppSettings settings;
    return settings;
}

DateFormat AppSettings::dateFormat() const
{
    QSettings settings("Fitlyzer", "FitlyzerC");
    const int rawValue = settings.value(dateFormatKey(), static_cast<int>(DateFormat::DD_MM_YYYY)).toInt();
    return sanitizeDateFormat(rawValue);
}

void AppSettings::setDateFormat(DateFormat format)
{
    QSettings settings("Fitlyzer", "FitlyzerC");
    settings.setValue(dateFormatKey(), static_cast<int>(format));
}

QString AppSettings::dateFormatKey()
{
    return QStringLiteral("general/dateFormat");
}

DateFormat AppSettings::sanitizeDateFormat(int rawValue)
{
    switch (rawValue)
    {
        case static_cast<int>(DateFormat::DD_MM_YYYY):
            return DateFormat::DD_MM_YYYY;
        case static_cast<int>(DateFormat::YYYY_MM_DD):
            return DateFormat::YYYY_MM_DD;
        case static_cast<int>(DateFormat::DD_DOT_MM_DOT_YYYY):
            return DateFormat::DD_DOT_MM_DOT_YYYY;
        case static_cast<int>(DateFormat::MM_DD_YYYY):
            return DateFormat::MM_DD_YYYY;
        default:
            return DateFormat::DD_MM_YYYY;
    }
}
