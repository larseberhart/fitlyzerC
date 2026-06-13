#pragma once

#include <QString>

#include "RideData.h"

class FitParser
{
public:
    RideData load(const QString& fileName);

private:
    static double semicirclesToDegrees(long value);
};