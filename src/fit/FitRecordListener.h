#pragma once

#include "RideData.h"

#include <fit_record_mesg_listener.hpp>

class FitRecordListener :
    public fit::RecordMesgListener
{
public:
    explicit FitRecordListener(
        RideData& rideData);

    void OnMesg(
        fit::RecordMesg& mesg) override;

    FIT_DATE_TIME firstTimestamp() const { return m_firstTimestamp; }
    FIT_DATE_TIME lastTimestamp() const { return m_lastTimestamp; }

private:
    RideData& m_rideData;

    FIT_DATE_TIME m_firstTimestamp = FIT_DATE_TIME_INVALID;
    FIT_DATE_TIME m_lastTimestamp = FIT_DATE_TIME_INVALID;
};