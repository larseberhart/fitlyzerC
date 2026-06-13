#pragma once

#include <vector>

struct RideRecord
{
    double elapsedSeconds = 0.0;

    double latitude = 0.0;
    double longitude = 0.0;

    double power = 0.0;
    double heartRate = 0.0;
    double cadence = 0.0;
    double speed = 0.0;
    double altitude = 0.0;

    bool hasGps = false;
    bool hasPower = false;
    bool hasHeartRate = false;
    bool hasCadence = false;
    bool hasSpeed = false;
    bool hasAltitude = false;
};

class RideData
{
public:
    std::vector<RideRecord> records;
};