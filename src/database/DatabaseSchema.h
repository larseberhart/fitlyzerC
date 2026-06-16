// SPDX-License-Identifier: GPL-3


#pragma once

#include <array>
#include <string_view>

namespace DatabaseSchema {

/**
 * @brief Current database schema version.
 */
static constexpr int kCurrentVersion = 8;

/**
 * @brief SQL DDL statements for database schema initialization.
 *
 * Each element is one CREATE TABLE or CREATE INDEX statement.
 * Statements are executed in order to initialize an empty database.
 */
inline constexpr std::array kStatements = {
    std::string_view{
        "CREATE TABLE IF NOT EXISTS schema_info ("
        "  version    INTEGER NOT NULL,"
        "  created_at TEXT    NOT NULL"
        ")"
    },
    std::string_view{
        "CREATE TABLE IF NOT EXISTS athletes ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  first_name    TEXT    NOT NULL,"
        "  last_name     TEXT    NOT NULL,"
        "  date_of_birth TEXT,"
        "  email         TEXT,"
        "  height_cm     REAL,"
        "  ftp           INTEGER NOT NULL DEFAULT 250,"
        "  created_at    TEXT    NOT NULL"
        ")"
    },
    std::string_view{
        "CREATE TABLE IF NOT EXISTS ftp_history ("
        "  id             INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  athlete_id     INTEGER NOT NULL REFERENCES athletes(id) ON DELETE CASCADE,"
        "  ftp_watts      INTEGER NOT NULL,"
        "  effective_from TEXT    NOT NULL,"
        "  notes          TEXT"
        ")"
    },
    std::string_view{
        "CREATE TABLE IF NOT EXISTS weight_history ("
        "  id             INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  athlete_id     INTEGER NOT NULL REFERENCES athletes(id) ON DELETE CASCADE,"
        "  weight_kg      REAL    NOT NULL,"
        "  effective_from TEXT    NOT NULL"
        ")"
    },
    std::string_view{
        "CREATE TABLE IF NOT EXISTS activities ("
        "  id               INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  athlete_id       INTEGER NOT NULL REFERENCES athletes(id) ON DELETE CASCADE,"
        "  fit_hash         TEXT    UNIQUE,"
        "  file_name        TEXT,"
        "  sport            TEXT,"
        "  activity_start_time TEXT NOT NULL,"
        "  end_time         TEXT,"
        "  duration_sec     REAL,"
        "  distance_m       REAL,"
        "  avg_power        REAL,"
        "  max_power        REAL,"
        "  normalized_power REAL,"
        "  avg_hr           REAL,"
        "  max_hr           REAL,"
        "  avg_cadence      REAL,"
        "  avg_speed        REAL,"
        "  elevation_gain   REAL,"
        "  notes            TEXT,"
        "  weather_notes    TEXT,"
        "  temperature      REAL,"
        "  weather          TEXT,"
        "  wind             TEXT,"
        "  rpe              INTEGER,"
        "  fatigue          INTEGER,"
        "  sleep            REAL,"
        "  weight           REAL,"
        "  bike             TEXT,"
        "  equipment        TEXT,"
        "  import_time      TEXT    NOT NULL,"
        "  analysis_version INTEGER NOT NULL DEFAULT 1,"
        "  fingerprint      TEXT,"
        "  analysis_flags   INTEGER NOT NULL DEFAULT 0,"
        "  import_source    TEXT"
        ")"
    },
    std::string_view{
        "CREATE INDEX IF NOT EXISTS idx_activities_activity_start_time"
        "  ON activities(activity_start_time DESC)"
    },
    std::string_view{
        "CREATE INDEX IF NOT EXISTS idx_activities_athlete_start_time"
        "  ON activities(athlete_id, activity_start_time DESC)"
    },
    std::string_view{
        "CREATE TABLE IF NOT EXISTS activity_samples ("
        "  id                INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  activity_id       INTEGER NOT NULL REFERENCES activities(id) ON DELETE CASCADE,"
        "  elapsed_seconds   REAL    NOT NULL,"
        "  power_total       REAL,"
        "  heart_rate        REAL,"
        "  cadence           REAL,"
        "  speed             REAL,"
        "  latitude          REAL,"
        "  longitude         REAL,"
        "  altitude          REAL,"
        "  has_gps           INTEGER NOT NULL DEFAULT 0,"
        "  has_power         INTEGER NOT NULL DEFAULT 0,"
        "  has_heart_rate    INTEGER NOT NULL DEFAULT 0,"
        "  has_cadence       INTEGER NOT NULL DEFAULT 0,"
        "  has_speed         INTEGER NOT NULL DEFAULT 0,"
        "  has_altitude      INTEGER NOT NULL DEFAULT 0"
        ")"
    },
    std::string_view{
        "CREATE INDEX IF NOT EXISTS idx_activity_samples_activity_id"
        "  ON activity_samples(activity_id)"
    },
    std::string_view{
        "CREATE INDEX IF NOT EXISTS idx_activity_samples_time"
        "  ON activity_samples(activity_id, elapsed_seconds)"
    },
    std::string_view{
        "CREATE TABLE IF NOT EXISTS laps ("
        "  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  activity_id  INTEGER NOT NULL REFERENCES activities(id) ON DELETE CASCADE,"
        "  lap_number   INTEGER NOT NULL,"
        "  start_time   TEXT,"
        "  duration_sec REAL,"
        "  distance_m   REAL,"
        "  avg_power    REAL,"
        "  avg_hr       REAL"
        ")"
    },
    std::string_view{
        "CREATE TABLE IF NOT EXISTS tags ("
        "  id   INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL UNIQUE"
        ")"
    },
    std::string_view{
        "CREATE TABLE IF NOT EXISTS activity_tags ("
        "  activity_id INTEGER NOT NULL REFERENCES activities(id) ON DELETE CASCADE,"
        "  tag_id      INTEGER NOT NULL REFERENCES tags(id)     ON DELETE CASCADE,"
        "  PRIMARY KEY (activity_id, tag_id)"
        ")"
    },
    std::string_view{
        "CREATE TABLE IF NOT EXISTS intervals ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  activity_id   INTEGER NOT NULL REFERENCES activities(id) ON DELETE CASCADE,"
        "  start_sample  INTEGER NOT NULL,"
        "  end_sample    INTEGER NOT NULL,"
        "  name          TEXT,"
        "  type          TEXT,"
        "  avg_power     REAL,"
        "  np            REAL,"
        "  avg_hr        REAL,"
        "  avg_cadence   REAL,"
        "  notes         TEXT,"
        "  source        TEXT    NOT NULL DEFAULT 'auto',"
        "  locked        INTEGER NOT NULL DEFAULT 0,"
        "  updated_at    TEXT,"
        "  algorithm_version INTEGER NOT NULL DEFAULT 1,"
        "  deleted       INTEGER NOT NULL DEFAULT 0,"
        "  uuid          TEXT"
        ")"
    },
    std::string_view{
        "CREATE TABLE IF NOT EXISTS climbs ("
        "  id               INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  activity_id      INTEGER NOT NULL REFERENCES activities(id) ON DELETE CASCADE,"
        "  start_seconds    REAL    NOT NULL,"
        "  end_seconds      REAL    NOT NULL,"
        "  name             TEXT,"
        "  elevation_gain_m REAL,"
        "  length_km        REAL,"
        "  average_gradient REAL,"
        "  source           TEXT    NOT NULL DEFAULT 'auto',"
        "  locked           INTEGER NOT NULL DEFAULT 0,"
        "  created_at       TEXT    DEFAULT CURRENT_TIMESTAMP,"
        "  updated_at       TEXT    DEFAULT CURRENT_TIMESTAMP,"
        "  algorithm_version       INTEGER NOT NULL DEFAULT 1,"
        "  deleted                 INTEGER NOT NULL DEFAULT 0,"
        "  uuid                    TEXT,"
        "  original_start_seconds  REAL,"
        "  original_end_seconds    REAL,"
        "  avg_power               REAL,"
        "  np                      REAL,"
        "  avg_hr                  REAL,"
        "  avg_cadence             REAL,"
        "  vam                     REAL,"
        "  notes                   TEXT,"
        "  favorite                INTEGER NOT NULL DEFAULT 0,"
        "  rating                  INTEGER NOT NULL DEFAULT 0"
        ")"
    },
    std::string_view{
        "CREATE INDEX IF NOT EXISTS idx_climbs_activity_id"
        "  ON climbs(activity_id)"
    },
    std::string_view{
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_activity_filehash"
        "  ON activities(fit_hash) WHERE fit_hash IS NOT NULL"
    },
    std::string_view{
        "CREATE INDEX IF NOT EXISTS idx_activity_start_time"
        "  ON activities(activity_start_time)"
    },
    std::string_view{
        "CREATE INDEX IF NOT EXISTS idx_activity_end_time"
        "  ON activities(end_time)"
    },
    std::string_view{
        "CREATE INDEX IF NOT EXISTS idx_intervals_activity"
        "  ON intervals(activity_id)"
    },
    std::string_view{
        "CREATE TABLE IF NOT EXISTS planned_workouts ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  athlete_id    INTEGER NOT NULL REFERENCES athletes(id) ON DELETE CASCADE,"
        "  workout_date  TEXT    NOT NULL,"
        "  title         TEXT    NOT NULL,"
        "  type          TEXT,"
        "  duration_sec  REAL,"
        "  target_tss    REAL,"
        "  notes         TEXT"
        ")"
    },
    std::string_view{
        "CREATE INDEX IF NOT EXISTS idx_workouts_athlete_date"
        "  ON planned_workouts(athlete_id, workout_date)"
    },
    std::string_view{
        "CREATE TABLE IF NOT EXISTS daily_metrics ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  athlete_id    INTEGER NOT NULL REFERENCES athletes(id) ON DELETE CASCADE,"
        "  metric_date   TEXT    NOT NULL,"
        "  atl           REAL    NOT NULL DEFAULT 0,"
        "  ctl           REAL    NOT NULL DEFAULT 0,"
        "  tsb           REAL    NOT NULL DEFAULT 0"
        ")"
    },
    std::string_view{
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_daily_metrics_athlete_date"
        "  ON daily_metrics(athlete_id, metric_date)"
    },
    std::string_view{
        "CREATE TABLE IF NOT EXISTS equipment ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  athlete_id    INTEGER NOT NULL REFERENCES athletes(id) ON DELETE CASCADE,"
        "  name          TEXT    NOT NULL,"
        "  category      TEXT    NOT NULL,"
        "  service_interval_km REAL,"
        "  service_interval_hours REAL,"
        "  notes         TEXT"
        ")"
    },
    std::string_view{
        "CREATE TABLE IF NOT EXISTS equipment_usage ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  equipment_id  INTEGER NOT NULL REFERENCES equipment(id) ON DELETE CASCADE,"
        "  activity_id   INTEGER NOT NULL REFERENCES activities(id) ON DELETE CASCADE,"
        "  distance_km   REAL,"
        "  duration_sec  REAL"
        ")"
    },
    std::string_view{
        "CREATE TABLE IF NOT EXISTS imports ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  file_name     TEXT    NOT NULL,"
        "  imported_at   TEXT    NOT NULL,"
        "  status        TEXT    NOT NULL,"
        "  activity_id   INTEGER,"
        "  error_message TEXT"
        ")"
    },
    std::string_view{
        "CREATE INDEX IF NOT EXISTS idx_imports_date"
        "  ON imports(imported_at)"
    },
        std::string_view{
            "CREATE TABLE IF NOT EXISTS activity_analysis_settings ("
            "  activity_id              INTEGER PRIMARY KEY,"
            "  climb_min_gain           REAL,"
            "  climb_min_length         REAL,"
            "  climb_min_gradient       REAL,"
            "  interval_work_threshold  REAL,"
            "  interval_rest_threshold  REAL,"
            "  FOREIGN KEY(activity_id) REFERENCES activities(id) ON DELETE CASCADE"
            ")"
        },
        std::string_view{
            "CREATE TABLE IF NOT EXISTS activity_analysis_cache ("
            "  activity_id  INTEGER NOT NULL,"
            "  cache_key    TEXT    NOT NULL,"
            "  cache_value  BLOB,"
            "  PRIMARY KEY (activity_id, cache_key)"
            ")"
        },
    // Enable foreign-key enforcement (must be per-connection, set here via a dummy table)
    std::string_view{
        "PRAGMA foreign_keys = ON"
    },
};

} // namespace DatabaseSchema