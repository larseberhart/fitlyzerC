// SPDX-License-Identifier: GPL-3

/**
 * @file DateFormatter.h
 * @brief Application settings support for DateFormatter.
 *
 * Defines settings and formatting helpers used to configure application behavior and presentation.
 *
 * Responsibilities:
 * - Provide settings persistence or settings-related utilities
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QDate>
#include <QDateTime>
#include <QString>

/**
 * @brief Provides date/time formatting consistent with user settings.
 */
class DateFormatter
{
public:
    /**
     * @brief Formats a date according to current settings.
     * @param date Date to format.
     * @return Formatted date string.
     */
    static QString formatDate(const QDate& date);

    /**
     * @brief Formats a date/time according to current settings.
     * @param dateTime Date/time to format.
     * @return Formatted date/time string.
     */
    static QString formatDateTime(const QDateTime& dateTime);

    /**
     * @brief Formats a date as ISO 8601.
     * @param date Date to format.
     * @return ISO date string (YYYY-MM-DD).
     */
    static QString toIsoDate(const QDate& date);

    /**
     * @brief Formats a date/time as ISO 8601.
     * @param dateTime Date/time to format.
     * @return ISO date/time string.
     */
    static QString toIsoDateTime(const QDateTime& dateTime);

    /**
     * @brief Parses a date string.
     * @param text Date string to parse.
     * @return Parsed QDate.
     */
    static QDate parseDate(const QString& text);

    /**
     * @brief Gets the Qt date format pattern.
     * @return Format pattern string.
     */
    static QString qtDatePattern();
};