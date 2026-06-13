#pragma once

#include <array>
#include <string>

#include "fit/RideData.h"

struct PowerZone
{
    int         number;
    std::string name;
    double      lowerPct;       // lower bound as fraction of FTP (inclusive)
    double      upperPct;       // upper bound (exclusive); -1.0 = unbounded
    double      seconds = 0.0;  // time spent in this zone
};

class FTPZones
{
public:
    static constexpr int kZoneCount = 7;
    using ZoneArray = std::array<PowerZone, kZoneCount>;

    static ZoneArray defaultZones();
    static ZoneArray compute(const RideData& rideData, double ftp);
};
