#include "FitParser.h"
#include "FitRecordListener.h"

#include <fstream>
#include <cmath>
#include <numbers>

#include <fit_decode.hpp>
#include <fit_mesg_broadcaster.hpp>

// Remove GPS points that imply speed > 150 km/h (equirectangular approximation)
static void filterGpsOutliers(RideData& rideData)
{
    constexpr double kMaxSpeedKmh = 150.0;

    bool   prevValid = false;
    double prevLat = 0, prevLon = 0, prevSec = 0;

    for (auto& r : rideData.records)
    {
        if (!r.hasGps) continue;

        if (prevValid)
        {
            const double dt = r.elapsedSeconds - prevSec;
            if (dt > 0.0)
            {
                const double dLat = (r.latitude  - prevLat) * std::numbers::pi_v<double> / 180.0;
                const double dLon = (r.longitude - prevLon) * std::numbers::pi_v<double> / 180.0;
                const double mLat = (r.latitude  + prevLat) / 2.0 * std::numbers::pi_v<double> / 180.0;
                const double distKm =
                    std::hypot(dLat, dLon * std::cos(mLat)) * 6371.0;
                const double speedKmh = distKm / (dt / 3600.0);

                if (speedKmh > kMaxSpeedKmh)
                {
                    r.hasGps = false;
                    continue;
                }
            }
        }

        prevValid = true;
        prevLat   = r.latitude;
        prevLon   = r.longitude;
        prevSec   = r.elapsedSeconds;
    }
}

RideData FitParser::load(const QString& fileName)
{
    RideData rideData;

    std::ifstream fitFile(fileName.toStdString(), std::ios::binary);
    if (!fitFile.is_open())
        throw std::runtime_error("Cannot open FIT file: " +
                                 fileName.toStdString());

    fit::Decode decode;
    fit::MesgBroadcaster broadcaster;

    FitRecordListener listener(rideData);
    broadcaster.AddListener((fit::RecordMesgListener&)listener);

    if (!decode.CheckIntegrity(fitFile))
        throw std::runtime_error(
            "FIT integrity check failed.\n"
            "The file may be corrupted or was not fully written.");

    fitFile.clear();
    fitFile.seekg(0);

    decode.Read(fitFile, broadcaster, broadcaster);

    if (rideData.records.empty())
        throw std::runtime_error(
            "No activity records found in this FIT file.\n"
            "The file may be a device configuration or course file, not an activity.");

    filterGpsOutliers(rideData);

    return rideData;
}
