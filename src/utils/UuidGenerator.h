// SPDX-License-Identifier: GPL-3

/**
 * @file UuidGenerator.h
 * @brief Utility support component for UuidGenerator.
 *
 * Provides shared utility helpers used by multiple application subsystems.
 *
 * Responsibilities:
 * - Provide reusable utility behavior for common tasks
 *
 * @author Lars EBERHART
 */

#pragma once

#include <QString>
#include <QUuid>

namespace UuidGenerator
{
    /**
     * @brief Generates a new random UUID string.
     * @return UUID string without braces.
     */
    inline QString create()
    {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
}