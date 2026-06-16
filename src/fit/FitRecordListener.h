// SPDX-License-Identifier: GPL-3


#pragma once

#include "RideData.h"

#include <fit_record_mesg_listener.hpp>

/**
 * @brief Listener for FIT record messages during parsing.
 *
 * Processes individual FIT record messages and accumulates sample data into RideData.
 */
class FitRecordListener :
    public fit::RecordMesgListener
{
public:
    /**
     * @brief Constructs listener with reference to ride data accumulator.
     * @param rideData Reference to RideData object to populate.
     */
    explicit FitRecordListener(
        RideData& rideData);

    /**
     * @brief Processes a single FIT record message.
     * @param mesg FIT record message.
     */
    void OnMesg(
        fit::RecordMesg& mesg) override;

    /**
     * @brief Gets the first timestamp from records.
     * @return First timestamp value.
     */
    FIT_DATE_TIME firstTimestamp() const { return m_firstTimestamp; }

    /**
     * @brief Gets the last timestamp from records.
     * @return Last timestamp value.
     */
    FIT_DATE_TIME lastTimestamp() const { return m_lastTimestamp; }

private:
    RideData& m_rideData;

    FIT_DATE_TIME m_firstTimestamp = FIT_DATE_TIME_INVALID;
    FIT_DATE_TIME m_lastTimestamp = FIT_DATE_TIME_INVALID;
};