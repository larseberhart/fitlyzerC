#pragma once

#include <string>
#include <vector>

#include "fit/RideData.h"

struct Interval
{
    double startSeconds     = 0.0;
    double endSeconds       = 0.0;
    double durationSeconds  = 0.0;

    double averagePower     = 0.0;
    double maximumPower     = 0.0;
    double normalizedPower  = 0.0;

    double averageHeartRate = 0.0;
    double averageCadence   = 0.0;
    double averageSpeed     = 0.0;

    std::string label;   // "Work" or "Recovery"
};

// Kept at namespace scope so default-argument `Config{}` compiles cleanly.
struct IntervalDetectorConfig
{
    double ftp                = 250.0;
    double workThresholdPct   = 0.76;    // fraction of FTP for "work"
    double minWorkSeconds     = 60.0;    // minimum work interval duration
    double minRecoverySeconds = 20.0;    // minimum recovery duration
    int    smoothingWindow    = 30;      // rolling average width (samples)
};

class IntervalDetector
{
public:
    using Config = IntervalDetectorConfig;

    static std::vector<Interval> detect(
        const RideData& rideData,
        const Config& config = Config{});

private:
    static std::vector<double> smoothPower(
        const RideData& rideData, int window);

    static Interval buildInterval(
        const RideData& rideData,
        int startIdx, int endIdx,
        const std::string& label);
};

