// SPDX-License-Identifier: GPL-3

#pragma once

#include "BasePage.h"

/**
 * @brief Top-level page for video creation and export.
 *
 * Layout:
 *   - Preview panel
 *   - Render settings
 *   - Output settings
 *
 * Toolbar actions: Preview, Render, Export.
 *
 * Currently wraps the existing "Create Video" dialog workflow.
 * Future work: inline preview, render-progress bar, batch export.
 */
class VideoPage : public BasePage
{
    Q_OBJECT

public:
    explicit VideoPage(QWidget* parent = nullptr);

signals:
    /// @brief Emitted when the user requests a preview workflow.
    void previewRequested();

    /// @brief Emitted when the user requests a render workflow.
    void renderRequested();

    /// @brief Emitted when the user requests an export workflow.
    void exportRequested();
};
