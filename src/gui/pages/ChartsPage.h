// SPDX-License-Identifier: GPL-3

#pragma once

#include "BasePage.h"

class QWidget;

/**
 * @brief Top-level page for activity chart inspection.
 *
 * Hosts the ride charts (Power / HR / Cadence / Speed / Altitude),
 * the route map, and the intervals / laps / notes panels.
 * Map controls are kept isolated from chart controls on this page.
 *
 * Chart controls (metrics, smoothing, presets, zoom) live in the toolbar
 * area at the top of this page.  Map controls (style, fit-to-track, zoom)
 * are embedded in the map panel itself.
 */
class ChartsPage : public BasePage
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the Charts page.
     * @param colorBarWidget  Widget containing the "Color By" metric control and
     *                        the color-legend strip.
     * @param contentWidget   Pre-assembled chart+map+intervals widget.
     * @param parent          Parent widget.
     */
    explicit ChartsPage(QWidget* colorBarWidget,
                        QWidget* contentWidget,
                        QWidget* parent = nullptr);
};
