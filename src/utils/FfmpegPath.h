// SPDX-License-Identifier: GPL-3

/**
 * @file FfmpegPath.h
 * @brief Utility support component for FFmpeg path resolution.
 *
 * Provides platform-specific FFmpeg executable path detection.
 *
 * Responsibilities:
 * - Locate FFmpeg executable on system
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QString>

/**
 * @brief Resolves the path to the FFmpeg executable.
 * @return Full path to ffmpeg executable, or empty string if not found.
 */
QString resolveFfmpegExecutablePath();