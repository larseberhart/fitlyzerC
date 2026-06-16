// SPDX-License-Identifier: GPL-3

#pragma once

#include <QString>

/**
 * @brief Represents a single background FIT import request.
 */
struct ImportJob
{
    /// @brief Processing stage used for worker/queue state reporting.
    enum class State
    {
        Queued,    ///< Waiting in queue.
        Parsing,   ///< FIT file parsing and duplicate checks.
        Storing,   ///< Activity/sample persistence in database.
        Completed, ///< Successfully imported.
        Failed     ///< Failed or rejected as duplicate.
    };

    QString jobId;         ///< Unique import job id.
    QString batchId;       ///< Caller-defined batch id for grouped summaries.
    QString filePath;      ///< Absolute path to FIT file.
    QString athleteId;     ///< Target athlete id.
    QString importSource;  ///< Source tag (e.g. manual, drag_drop, watch_folder).
    State state = State::Queued;
    double progress = 0.0; ///< Best-effort progress percentage [0..100].
    QString errorMessage;  ///< User-facing failure details when state is Failed.
};
