// SPDX-License-Identifier: GPL-3

#include "ImportStatusWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>

#include "import/ImportProgressModel.h"

ImportStatusWidget::ImportStatusWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(6);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setMinimumWidth(120);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setMinimumWidth(150);
    m_progressBar->setTextVisible(true);

    m_detailLabel = new QLabel(this);
    m_detailLabel->setMinimumWidth(220);

    row->addWidget(m_titleLabel);
    row->addWidget(m_progressBar);
    row->addWidget(m_detailLabel);

    hide();
}

void ImportStatusWidget::setModel(ImportProgressModel* model)
{
    if (m_model == model)
        return;

    if (m_model)
        disconnect(m_model, nullptr, this, nullptr);

    m_model = model;
    if (m_model)
        connect(m_model, &ImportProgressModel::changed, this, &ImportStatusWidget::updateFromModel);

    updateFromModel();
}

void ImportStatusWidget::updateFromModel()
{
    if (!m_model || m_model->activeJobs() <= 0)
    {
        hide();
        return;
    }

    show();

    const int remaining = m_model->activeJobs();
    const QString plural = remaining == 1 ? QStringLiteral("file") : QStringLiteral("files");
    m_titleLabel->setText(QString("Importing %1 %2...").arg(remaining).arg(plural));
    m_progressBar->setValue(static_cast<int>(m_model->progress()));
    m_detailLabel->setText(
        QString("Current: %1 - %2")
            .arg(m_model->currentFileName().isEmpty() ? QStringLiteral("-") : m_model->currentFileName())
            .arg(m_model->statusText().isEmpty() ? QStringLiteral("Working") : m_model->statusText()));
}
