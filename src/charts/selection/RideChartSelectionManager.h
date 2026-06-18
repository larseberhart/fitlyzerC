// SPDX-License-Identifier: GPL-3

#pragma once

#include <QPointF>

#include "analysis/ClimbMetrics.h"

class QChart;
class QLineSeries;
class QValueAxis;

namespace RideChartSelectionManager
{
struct ClimbHandleHitResult
{
    bool hit = false;
    bool startHandle = true;
};

ClimbHandleHitResult hitTestClimbHandles(
    QChart* chart,
    QLineSeries* series,
    const QValueAxis* axisY,
    const Climb& climb,
    const QPointF& scenePos);

double climbHandleCenterOffsetYPx();
double climbHandleRadiusPx();
}
