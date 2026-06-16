// SPDX-License-Identifier: GPL-3


#include "FitParser.h"
#include "FitRecordListener.h"

#include <QDateTime>
#include <QTimeZone>

#include <fstream>
#include <cmath>
#include <numbers>

#include <fit_decode.hpp>
#include <fit_file_id_mesg_listener.hpp>
#include <fit_mesg_broadcaster.hpp>
#include <fit_session_mesg_listener.hpp>

namespace
{
QString fitTimestampToIsoUtc(FIT_DATE_TIME timestamp)
{
    if (timestamp == FIT_DATE_TIME_INVALID)
        return {};

    // FIT epoch: 1989-12-31T00:00:00Z.
    static constexpr qint64 kFitEpochToUnixOffset = 631065600;
    const qint64 unixSeconds = kFitEpochToUnixOffset + static_cast<qint64>(timestamp);
    return QDateTime::fromSecsSinceEpoch(unixSeconds, QTimeZone::UTC).toString(Qt::ISODate);
}

class FitSessionListener final : public fit::SessionMesgListener
{
public:
    void OnMesg(fit::SessionMesg& mesg) override
    {
        if (mesg.IsStartTimeValid())
            m_startTime = mesg.GetStartTime();
    }

    FIT_DATE_TIME startTime() const { return m_startTime; }

private:
    FIT_DATE_TIME m_startTime = FIT_DATE_TIME_INVALID;
};

class FitFileIdListener final : public fit::FileIdMesgListener
{
public:
    void OnMesg(fit::FileIdMesg& mesg) override
    {
        if (mesg.IsTimeCreatedValid())
            m_timeCreated = mesg.GetTimeCreated();
    }

    FIT_DATE_TIME timeCreated() const { return m_timeCreated; }

private:
    FIT_DATE_TIME m_timeCreated = FIT_DATE_TIME_INVALID;
};
}

// Remove GPS points that imply speed > 150 km/h (equirectangular approximation)
static void filterGpsOutliers(RideData& rideData)
{
    constexpr double kMaxSpeedKmh = 150.0;
    constexpr double kMinSpeedKmh = 0.0;

    bool   prevValid = false;
    double prevLat = 0, prevLon = 0, prevSec = 0;

    for (auto& r : rideData.records)
    {
        if (!r.hasGps) 
        {
            prevValid = false;
            continue;
        }

        // Additional sanity check on GPS coordinates
        if (r.latitude < -90.0 || r.latitude > 90.0 ||
            r.longitude < -180.0 || r.longitude > 180.0)
        {
            r.hasGps = false;
            prevValid = false;
            continue;
        }

        if (prevValid)
        {
            const double dt = r.elapsedSeconds - prevSec;
            if (dt > 0.0 && dt < 3600.0)  // Sanity: max 1 hour between samples
            {
                const double dLat = (r.latitude  - prevLat) * std::numbers::pi_v<double> / 180.0;
                const double dLon = (r.longitude - prevLon) * std::numbers::pi_v<double> / 180.0;
                const double mLat = (r.latitude  + prevLat) / 2.0 * std::numbers::pi_v<double> / 180.0;
                const double distKm =
                    std::hypot(dLat, dLon * std::cos(mLat)) * 6371.0;
                const double speedKmh = distKm / (dt / 3600.0);

                if (speedKmh > kMaxSpeedKmh || speedKmh < kMinSpeedKmh)
                {
                    r.hasGps = false;
                    prevValid = false;
                    continue;
                }
            }
            else if (dt <= 0.0 || dt >= 3600.0)
            {
                // Invalid time jump; don't trust this GPS point
                r.hasGps = false;
                prevValid = false;
                continue;
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
    FitSessionListener sessionListener;
    FitFileIdListener fileIdListener;
    broadcaster.AddListener((fit::RecordMesgListener&)listener);
    broadcaster.AddListener((fit::SessionMesgListener&)sessionListener);
    broadcaster.AddListener((fit::FileIdMesgListener&)fileIdListener);

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

    // Count records with power or heart rate data (key fields for cycling analysis)
    int recordsWithData = 0;
    for (const auto& r : rideData.records)
    {
        if (r.hasPower || r.hasHeartRate)
            ++recordsWithData;
    }

    // If less than 10% of records have key data, file is likely invalid
    if (recordsWithData == 0 || recordsWithData < static_cast<int>(rideData.records.size() * 0.1))
        throw std::runtime_error(
            "Activity file contains insufficient data.\n"
            "Expected power or heart rate data in most samples.");

    filterGpsOutliers(rideData);

    const FIT_DATE_TIME sessionStart = sessionListener.startTime();
    const FIT_DATE_TIME firstRecordTime = listener.firstTimestamp();
    const FIT_DATE_TIME lastRecordTime = listener.lastTimestamp();
    const FIT_DATE_TIME fileTime = fileIdListener.timeCreated();

    FIT_DATE_TIME chosenStart = sessionStart;
    if (chosenStart == FIT_DATE_TIME_INVALID)
        chosenStart = firstRecordTime;
    if (chosenStart == FIT_DATE_TIME_INVALID)
        chosenStart = fileTime;

    rideData.activityStartTimeUtc = fitTimestampToIsoUtc(chosenStart);
    rideData.activityEndTimeUtc = fitTimestampToIsoUtc(lastRecordTime != FIT_DATE_TIME_INVALID ? lastRecordTime : chosenStart);
    rideData.fileTimestampUtc = fitTimestampToIsoUtc(fileTime);

    if (rideData.activityStartTimeUtc.isEmpty())
        rideData.activityStartTimeUtc = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    if (rideData.activityEndTimeUtc.isEmpty())
        rideData.activityEndTimeUtc = rideData.activityStartTimeUtc;

    return rideData;
}