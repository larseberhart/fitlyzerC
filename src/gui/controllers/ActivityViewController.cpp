// SPDX-License-Identifier: GPL-3

#include "ActivityViewController.h"

#include "../WorkoutController.h"
#include "../../database/DatabaseManager.h"
#include "../../database/ActivityRepository.h"
#include "../../database/AthleteRepository.h"
#include "../../core/settings/DateFormatter.h"
#include "../AthleteHeaderWidget.h"

#include <QLabel>
#include <QFileInfo>

/**
 * @brief Constructs the activity view controller.
 * @param controller Pointer to WorkoutController for data access.
 * @param dbManager Pointer to DatabaseManager for queries.
 * @param parent Parent object.
 */
ActivityViewController::ActivityViewController(
    WorkoutController* controller,
    DatabaseManager* dbManager,
    QObject* parent)
    : QObject(parent)
    , m_controller(controller)
    , m_dbManager(dbManager)
{
    // Listen to workout controller signals
    if (m_controller)
    {
        connect(m_controller, &WorkoutController::workoutLoaded,
                this, &ActivityViewController::onWorkoutLoaded);
    }
}

/**
 * @brief Destructor.
 */
ActivityViewController::~ActivityViewController() = default;

/**
 * @brief Sets the athlete context information.
 */
void ActivityViewController::setAthleteContext(int athleteId, const QString& athleteName)
{
    m_currentAthleteId = athleteId;
    m_currentAthleteName = athleteName;
}

/**
 * @brief Sets the athlete header widget for display updates.
 */
void ActivityViewController::setAthleteHeaderWidget(AthleteHeaderWidget* athleteHeader)
{
    m_athleteHeader = athleteHeader;
}

/**
 * @brief Sets the status bar widgets for updates.
 */
void ActivityViewController::setStatusBarLabels(
    QLabel* dbStatusLabel,
    QLabel* athleteStatusLabel,
    QLabel* activityCountLabel)
{
    m_dbStatusLabel = dbStatusLabel;
    m_athleteStatusLabel = athleteStatusLabel;
    m_activityCountLabel = activityCountLabel;
}

/**
 * @brief Loads an activity by ID.
 * @param activityId ID of activity to load.
 * @param outError Output string for error message on failure.
 * @return True if successfully loaded, false otherwise.
 */
bool ActivityViewController::loadActivity(int activityId, QString& outError)
{
    if (!m_controller)
    {
        outError = "Activity controller not initialized.";
        return false;
    }

    if (!m_controller->loadActivity(activityId, outError))
        return false;

    emit activityLoaded(activityId);
    return true;
}

/**
 * @brief Refreshes the display of the currently loaded activity.
 */
void ActivityViewController::refreshActivity()
{
    updateActivityDisplay();
    emit activityDisplayUpdated();
}

/**
 * @brief Updates all activity-related UI elements.
 */
void ActivityViewController::updateActivityDisplay()
{
    updateActivityMetadata();
    populateActivityDetails();
    syncAnalysisViews();
}

/**
 * @brief Updates activity metadata and stats display.
 */
void ActivityViewController::updateStatsDisplay()
{
    updateActivityMetadata();
}

/**
 * @brief Updates status bar information.
 */
void ActivityViewController::updateStatusBarInformation()
{
    if (m_dbStatusLabel)
        m_dbStatusLabel->setText(
            m_dbManager && m_dbManager->isOpen()
                ? QString("Database: %1").arg(QFileInfo(m_dbManager->path()).fileName())
                : "Database: none");

    if (m_athleteStatusLabel)
    {
        m_athleteStatusLabel->setText(
            m_currentAthleteName.isEmpty()
                ? "Athlete: none"
                : QString("Athlete: %1").arg(m_currentAthleteName));
    }

    if (m_activityCountLabel)
    {
        int activityCount = 0;
        if (m_dbManager && m_dbManager->isOpen() && m_currentAthleteId > 0)
        {
            auto db = m_dbManager->database();
            ActivityRepository repo(db);
            activityCount = repo.listActivities(m_currentAthleteId).size();
        }
        m_activityCountLabel->setText(QString("Activities: %1").arg(activityCount));
    }
}

/**
 * @brief Slot for when WorkoutController loads a new activity.
 */
void ActivityViewController::onWorkoutLoaded()
{
    updateActivityDisplay();
    emit activityDisplayUpdated();
}

/**
 * @brief Helper to update activity metadata in UI.
 */
void ActivityViewController::updateActivityMetadata()
{
    if (!m_athleteHeader || !m_controller)
        return;

    // TODO: Extract full metadata update from MainWindow::updateStatsLabel
    // This requires access to:
    // - m_currentAthleteId
    // - m_currentAthleteName
    // - Activity database queries
    // For now, this is a placeholder
}

/**
 * @brief Helper to populate activity detail panels.
 */
void ActivityViewController::populateActivityDetails()
{
    // TODO: Extract detail panel population from MainWindow
}

/**
 * @brief Helper to sync charts and map with current activity.
 */
void ActivityViewController::syncAnalysisViews()
{
    // TODO: Extract analysis view synchronization from MainWindow
    // This will delegate to ChartController and MapController once available
}

/**
 * @brief Helper to format activity date for display.
 */
QString ActivityViewController::formatActivityDateLabel(const QString& rawDate) const
{
    QDate date = QDate::fromString(rawDate, Qt::ISODate);
    if (!date.isValid())
        return "-";
    return DateFormatter::toIsoDate(date);
}
