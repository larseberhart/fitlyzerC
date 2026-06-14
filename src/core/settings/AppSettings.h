#pragma once

#include <QString>

class QSettings;

enum class DateFormat
{
    DD_MM_YYYY = 0,
    YYYY_MM_DD,
    DD_DOT_MM_DOT_YYYY,
    MM_DD_YYYY
};

class AppSettings
{
public:
    static AppSettings& instance();

    DateFormat dateFormat() const;
    void setDateFormat(DateFormat format);

    int tileCacheSize() const;   // number of tiles to keep in RAM (128-2048)
    void setTileCacheSize(int tiles);

private:
    AppSettings() = default;

    static QString dateFormatKey();
    static DateFormat sanitizeDateFormat(int rawValue);
    static QString tileCacheSizeKey();
};
