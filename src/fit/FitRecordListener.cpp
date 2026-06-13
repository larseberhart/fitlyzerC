#include "FitRecordListener.h"

FitRecordListener::FitRecordListener(
    RideData& rideData)
    : m_rideData(rideData)
{
}

void FitRecordListener::OnMesg(
    fit::RecordMesg& mesg)
{
    RideRecord record;

    if (mesg.IsPositionLatValid() &&
        mesg.IsPositionLongValid())
    {
        record.latitude =
            mesg.GetPositionLat()
            * (180.0 / 2147483648.0);

        record.longitude =
            mesg.GetPositionLong()
            * (180.0 / 2147483648.0);

        record.hasGps = true;
    }

    if (mesg.IsHeartRateValid())
    {
        record.heartRate =
            mesg.GetHeartRate();

        record.hasHeartRate = true;
    }

    if (mesg.IsCadenceValid())
    {
        record.cadence =
            mesg.GetCadence();

        record.hasCadence = true;
    }

    if (mesg.IsPowerValid())
    {
        record.power =
            mesg.GetPower();

        record.hasPower = true;
    }

    if (mesg.IsAltitudeValid())
    {
        record.altitude =
            mesg.GetAltitude();

        record.hasAltitude = true;
    }

    if (mesg.IsSpeedValid())
    {
        record.speed =
            mesg.GetSpeed() * 3.6;

        record.hasSpeed = true;
    }

    if (mesg.IsTimestampValid())
    {
        FIT_DATE_TIME ts = mesg.GetTimestamp();

        if (m_firstTimestamp == FIT_DATE_TIME_INVALID)
            m_firstTimestamp = ts;

        record.elapsedSeconds =
            static_cast<double>(ts - m_firstTimestamp);
    }
    else if (!m_rideData.records.empty())
    {
        record.elapsedSeconds =
            m_rideData.records.back().elapsedSeconds + 1.0;
    }

    m_rideData.records.push_back(record);
}