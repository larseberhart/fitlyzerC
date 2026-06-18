// SPDX-License-Identifier: GPL-3

#pragma once

#include <QObject>
#include <QString>
#include <memory>

class WorkoutController;
class DatabaseManager;

/**
 * @brief Manages activity loading, switching, and display operations.
 *
 * Handles activity-specific UI updates including:
 * - Loading and switching between activities
 * - Refreshing activity display
 * - Updating activity metadata
 * - Populating activity detail panels
 */
class ActivityViewController : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the activity view controller.
     * @param controller Pointer to WorkoutController for data access.
     * @param dbManager Pointer to DatabaseManager for queries.
     * @param parent Parent object.
     */
    explicit ActivityViewController(
        WorkoutController* controller,
        DatabaseManager* dbManager,
        QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~ActivityViewController() override;

    /**
     * @brief Loads an activity by ID.
     * @param activityId ID of activity to load.
     * @param outError Output string for error message on failure.
     * @return True if successfully loaded, false otherwise.
     */
    bool loadActivity(int activityId, QString& outError);

    /**
     * @brief Refreshes the display of the currently loaded activity.
     */
    void refreshActivity();

    /**
     * @brief Updates all activity-related UI elements.
     *
     * Called after loading a new activity or when data changes.
     */
    void updateActivityDisplay();

    /**
     * @brief Sets the athlete context information.
     * @param athleteId Current athlete ID.
     * @param athleteName Current athlete name.
     */
    void setAthleteContext(int athleteId, const QString& athleteName);

    /**
     * @brief Sets the athlete header widget for display updates.
     * @param athleteHeader Pointer to athlete header widget.
     */
    void setAthleteHeaderWidget(class AthleteHeaderWidget* athleteHeader);

    /**
     * @brief Sets the status bar widgets for updates.
     * @param dbStatusLabel Label for database status.
     * @param athleteStatusLabel Label for athlete status.
     * @param activityCountLabel Label for activity count.
     */
    void setStatusBarLabels(
        class QLabel* dbStatusLabel,
        class QLabel* athleteStatusLabel,
        class QLabel* activityCountLabel);

    /**
     * @brief Updates activity metadata and stats display.
     */
    void updateStatsDisplay();

    /**
     * @brief Updates status bar information.
     */
    void updateStatusBarInformation();

signals:
    /// @brief Emitted when an activity has been successfully loaded.
    void activityLoaded(int activityId);

    /// @brief Emitted when the activity display has been updated.
    void activityDisplayUpdated();

    /// @brief Emitted when an error occurs during activity operations.
    void errorOccurred(const QString& message);

private slots:
    /// @brief Slot for when WorkoutController loads a new activity.
    void onWorkoutLoaded();

private:
    /// @brief Helper to update activity metadata in UI.
    void updateActivityMetadata();

    /// @brief Helper to populate activity detail panels.
    void populateActivityDetails();

    /// @brief Helper to sync charts and map with current activity.
    void syncAnalysisViews();

    /// @brief Helper to format activity date for display.
    QString formatActivityDateLabel(const QString& rawDate) const;

    // UI widget pointers
    class AthleteHeaderWidget* m_athleteHeader = nullptr;
    class QLabel* m_dbStatusLabel = nullptr;
    class QLabel* m_athleteStatusLabel = nullptr;
    class QLabel* m_activityCountLabel = nullptr;

    // Athlete context
    int m_currentAthleteId = -1;
    QString m_currentAthleteName;

    WorkoutController* m_controller;
    DatabaseManager* m_dbManager;
};
