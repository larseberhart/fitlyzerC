#pragma once

#include "fit/RideData.h"

struct WorkoutStatistics
{
    double durationSeconds   = 0.0;   // total elapsed time
    double movingTimeSeconds = 0.0;   // time with speed > 2 km/h
    double totalDistanceKm   = 0.0;
    double elevationGainM    = 0.0;

    double averagePower      = 0.0;
    double maximumPower      = 0.0;

    double averageHeartRate  = 0.0;
    double maximumHeartRate  = 0.0;

    double averageCadence    = 0.0;
    double maximumCadence    = 0.0;

    double averageSpeed      = 0.0;
    double maximumSpeed      = 0.0;
};

class WorkoutAnalyzer
{
public:
    WorkoutStatistics analyze(const RideData& rideData);
};
