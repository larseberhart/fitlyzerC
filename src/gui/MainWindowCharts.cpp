// SPDX-License-Identifier: GPL-3

#include "MainWindow.h"

#include "controllers/ChartController.h"
#include "maps/MapRenderer.h"

void MainWindow::syncChartContextToController(bool includeLegendLayout)
{
    if (!m_chartController)
        return;

    const std::vector<Climb>& climbsForView =
        m_climbMinLengthSpin ? m_detectedClimbs : m_controller->climbs();

    m_chartController->setClimbs(climbsForView);
    m_chartController->setColorMetric(static_cast<int>(currentColorMetric()));
    m_chartController->setColorContext(buildColorContext());
    m_chartController->setAthleteId(m_currentAthleteId);
    m_chartController->setZonesTable(m_zonesTable);
    m_chartController->setChartControls(
        m_colorMetricCombo, m_powerSmoothingCombo,
        m_autoSmoothingCheck, m_climbOverlayEnabledCheck,
        m_climbOverlayMetricCombo,
        includeLegendLayout ? m_colorLegendLayout : nullptr);
    m_chartController->setAnalysisTabWidgets(
        m_analysisTabWidget,
        m_activityTabStack,
        m_zonesTabStack,
        m_histogramTabStack,
        m_powerCurveTabStack,
        m_calendarTabStack,
        m_fitnessTabStack);
    m_chartController->setClimbingTabStack(m_climbingTabStack);
}

void MainWindow::applyChartControlDrivenUpdates(bool includeLegendLayout, bool applySmoothing)
{
    if (!m_chartController)
        return;

    syncChartContextToController(includeLegendLayout);
    if (applySmoothing)
        m_chartController->applyChartSmoothing();
    m_chartController->updateColorLegend();
    m_chartController->updateZoneAvailability();
    m_chartController->updateCharts();
    m_chartController->updateZonesTab();
}

void MainWindow::applyChartAndMapUpdates(bool includeLegendLayout, bool applySmoothing)
{
    applyChartControlDrivenUpdates(includeLegendLayout, applySmoothing);
    refreshMapRideDataFromCurrentState();
}

void MainWindow::updateChartAnalysisEmptyStates()
{
    if (!m_chartController)
        return;

    syncChartContextToController(false);
    m_chartController->updateAnalysisEmptyStates();
}

void MainWindow::refreshMapRideDataFromCurrentState()
{
    if (!m_mapRenderer)
        return;

    m_mapRenderer->setRideData(m_controller->rideData(), currentColorMetric(), buildColorContext());
}
