// SPDX-License-Identifier: GPL-3

/**
 * @file Platform.h
 * @brief Platform abstraction component for Platform.
 *
 * Implements platform-specific helpers and abstractions used to adapt behavior across operating systems.
 *
 * Responsibilities:
 * - Provide platform-specific integration behavior
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QString>

/**
 * @brief Platform-specific abstractions and utilities.
 *
 * Provides OS-agnostic access to system paths and file operations,
 * with implementation specific to macOS, Linux, or Windows.
 */
namespace Platform
{
/**
 * @brief Returns the platform-specific application data directory path.
 * @return Full path to application data folder.
 */
QString applicationDataDirectory();

/**
 * @brief Returns the default directory for database files.
 * @return Full path to database directory.
 */
QString defaultDatabaseDirectory();

/**
 * @brief Returns the default database file path.
 * @return Full path to default database file.
 */
QString defaultDatabasePath();

/**
 * @brief Returns the default backup path for a database file.
 * @param databasePath Path to the original database file.
 * @return Suggested backup path.
 */
QString defaultBackupPath(const QString& databasePath);

/**
 * @brief Opens a path in the platform file manager.
 * @param path Path to file or directory to open.
 */
void openInFileManager(const QString& path);
}