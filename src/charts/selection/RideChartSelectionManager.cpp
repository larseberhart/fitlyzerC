// SPDX-License-Identifier: GPL-3

#include "RideChartSelectionManager.h"

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

namespace
{
constexpr double kClimbHandleCenterOffsetYPx = 10.0;
constexpr double kClimbHandleRadiusPx = 8.0;
constexpr double kClimbHandleGrabRadiusPx = 24.0;
}

namespace RideChartSelectionManager
{
ClimbHandleHitResult hitTestClimbHandles(
    QChart* chart,
    QLineSeries* series,
    const QValueAxis* axisY,
    const Climb& climb,
    const QPointF& scenePos)
{
    if (!chart || !series || !axisY)
        return {};

    const QRectF plotArea = chart->plotArea();
    if (!plotArea.contains(scenePos))
        return {};

    QPointF startPos = chart->mapToPosition(QPointF(climb.startSeconds / 60.0, axisY->min()), series);
    QPointF endPos = chart->mapToPosition(QPointF(climb.endSeconds / 60.0, axisY->min()), series);
    startPos.setY(plotArea.top() + kClimbHandleCenterOffsetYPx);
    endPos.setY(plotArea.top() + kClimbHandleCenterOffsetYPx);

    const double dxStart = scenePos.x() - startPos.x();
    const double dyStart = scenePos.y() - startPos.y();
    const double dxEnd = scenePos.x() - endPos.x();
    const double dyEnd = scenePos.y() - endPos.y();
    const double distStartSq = dxStart * dxStart + dyStart * dyStart;
    const double distEndSq = dxEnd * dxEnd + dyEnd * dyEnd;
    const double radiusSq = kClimbHandleGrabRadiusPx * kClimbHandleGrabRadiusPx;

    if (distStartSq <= radiusSq || distEndSq <= radiusSq)
        return { true, distStartSq <= distEndSq };

    return {};
}

double climbHandleCenterOffsetYPx()
{
    return kClimbHandleCenterOffsetYPx;
}

double climbHandleRadiusPx()
{
    return kClimbHandleRadiusPx;
}
}
