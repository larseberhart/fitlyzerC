#pragma once

#include <QString>
#include <QUuid>

namespace UuidGenerator
{
    /// Generate a new random UUID string (no braces).
    inline QString create()
    {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
}
