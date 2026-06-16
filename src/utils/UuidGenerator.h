// SPDX-License-Identifier: GPL-3


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