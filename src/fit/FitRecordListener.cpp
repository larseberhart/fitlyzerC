// SPDX-License-Identifier: GPL-3


#include "FitRecordListener.h"

#include <algorithm>

FitRecordListener::FitRecordListener(
    RideData& rideData)
    : m_rideData(rideData)
{
}

void FitRecordListener::OnMesg(
    fit::RecordMesg& mesg)
{
    RideRecord record;

    // GPS: validate coordinates are within valid range
    if (mesg.IsPositionLatValid() &&
        mesg.IsPositionLongValid())
    {
        const double lat = mesg.GetPositionLat() * (180.0 / 2147483648.0);
        const double lon = mesg.GetPositionLong() * (180.0 / 2147483648.0);

        if (lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0)
        {
            record.latitude = lat;
            record.longitude = lon;
            record.hasGps = true;
        }
    }

    // Heart Rate: valid range 0-255 bpm
    if (mesg.IsHeartRateValid())
    {
        const auto hr = mesg.GetHeartRate();
        if (hr >= 0 && hr <= 255)
        {
            record.heartRate = hr;
            record.hasHeartRate = true;
        }
    }

    // Cadence: valid range 0-255 rpm
    if (mesg.IsCadenceValid())
    {
        const auto cad = mesg.GetCadence();
        if (cad >= 0 && cad <= 255)
        {
            record.cadence = cad;
            record.hasCadence = true;
        }
    }

    // Power: valid range 0-5000 watts
    if (mesg.IsPowerValid())
    {
        const auto pow = mesg.GetPower();
        if (pow >= 0 && pow <= 5000)
        {
            record.power = pow;
            record.hasPower = true;
        }
    }

    // Altitude: valid range -500 to 10000 meters
    if (mesg.IsAltitudeValid())
    {
        const auto alt = mesg.GetAltitude();
        if (alt >= -500.0 && alt <= 10000.0)
        {
            record.altitude = alt;
            record.hasAltitude = true;
        }
    }

    // Speed: valid range 0-150 km/h (converted from m/s)
    if (mesg.IsSpeedValid())
    {
        const auto spd = mesg.GetSpeed();
        const double spdKmh = spd * 3.6;
        if (spdKmh >= 0.0 && spdKmh <= 150.0)
        {
            record.speed = spdKmh;
            record.hasSpeed = true;
        }
    }

    // Timestamp: validate monotonicity
    if (mesg.IsTimestampValid())
    {
        FIT_DATE_TIME ts = mesg.GetTimestamp();

        if (m_firstTimestamp == FIT_DATE_TIME_INVALID)
            m_firstTimestamp = ts;

        // Ensure monotonic timestamps; clamp to at least the last recorded elapsed time
        if (ts >= m_lastTimestamp)
        {
            m_lastTimestamp = ts;
            record.elapsedSeconds = static_cast<double>(ts - m_firstTimestamp);
        }
        else if (!m_rideData.records.empty())
        {
            // Backward timestamp: use previous elapsed + 1 second
            record.elapsedSeconds = m_rideData.records.back().elapsedSeconds + 1.0;
        }
        else
        {
            record.elapsedSeconds = 0.0;
        }
    }
    else if (!m_rideData.records.empty())
    {
        // No timestamp: continue sequence from last record
        record.elapsedSeconds =
            m_rideData.records.back().elapsedSeconds + 1.0;
    }

    // Only add records that have at least one valid field
    if (record.hasGps || record.hasPower || record.hasHeartRate ||
        record.hasCadence || record.hasSpeed || record.hasAltitude)
    {
        m_rideData.records.push_back(record);
    }
}