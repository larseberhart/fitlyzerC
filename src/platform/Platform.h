// SPDX-License-Identifier: GPL-3


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
QString applicationDataDirectory();

QString defaultDatabaseDirectory();

QString defaultDatabasePath();

QString defaultBackupPath(const QString& databasePath);

void openInFileManager(const QString& path);
}