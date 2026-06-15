// SPDX-License-Identifier: GPL-3

/**
 * @file AppSettings.h
 * @brief Application settings support for AppSettings.
 *
 * Defines settings and formatting helpers used to configure application behavior and presentation.
 *
 * Responsibilities:
 * - Provide settings persistence or settings-related utilities
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QString>

class QSettings;

/**
 * @brief Supported date display formats.
 */
enum class DateFormat
{
    DD_MM_YYYY = 0,
    YYYY_MM_DD,
    DD_DOT_MM_DOT_YYYY,
    MM_DD_YYYY
};

/**
 * @brief Application-wide settings singleton.
 *
 * Manages persistent user preferences including date formatting and cache sizes.
 */
class AppSettings
{
public:
    /**
     * @brief Returns the global settings instance.
     * @return Reference to the singleton AppSettings.
     */
    static AppSettings& instance();

    /**
     * @brief Gets the current date format preference.
     * @return Current DateFormat.
     */
    DateFormat dateFormat() const;

    /**
     * @brief Sets the date format preference.
     * @param format New DateFormat.
     */
    void setDateFormat(DateFormat format);

    /**
     * @brief Gets the tile cache size.
     * @return Number of tiles to keep in RAM (128-2048).
     */
    int tileCacheSize() const;

    /**
     * @brief Sets the tile cache size.
     * @param tiles Number of tiles to cache (128-2048).
     */
    void setTileCacheSize(int tiles);

private:
    AppSettings() = default;

    static QString dateFormatKey();
    static DateFormat sanitizeDateFormat(int rawValue);
    static QString tileCacheSizeKey();
};