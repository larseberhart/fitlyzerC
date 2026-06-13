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

private:
    RideData& m_rideData;

    FIT_DATE_TIME m_firstTimestamp = FIT_DATE_TIME_INVALID;
};