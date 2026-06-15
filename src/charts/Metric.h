// SPDX-License-Identifier: GPL-3

/**
 * @file Metric.h
 * @brief Chart widget and visualization support for Metric.
 *
 * Provides chart rendering or chart-related data types used to visualize ride and fitness metrics in the UI.
 *
 * Responsibilities:
 * - Provide chart visualization behavior or chart support types
 *
 * @author Lars EBERHART
 */

#pragma once

/**
 * @brief Chart display metrics.
 */
enum class Metric
{
    Power,
    HeartRate,
    Cadence,
    Speed,
    Altitude
};