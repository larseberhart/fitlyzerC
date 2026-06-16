// SPDX-License-Identifier: GPL-3

#pragma once

#include <QWidget>

class QLabel;
class QProgressBar;
class ImportProgressModel;

/**
 * @brief Toolbar widget showing live background import queue progress.
 */
class ImportStatusWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ImportStatusWidget(QWidget* parent = nullptr);

    /// @brief Binds a progress model; disconnects any previously bound model.
    void setModel(ImportProgressModel* model);

private:
    void updateFromModel();

    ImportProgressModel* m_model = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_detailLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
};
