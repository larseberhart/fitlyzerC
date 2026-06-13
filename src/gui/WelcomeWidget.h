#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

class WelcomeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomeWidget(QWidget* parent = nullptr);

    void setFirstLaunch(bool firstLaunch);

signals:
    void importRequested();
    void openDatabaseRequested();
    void createDatabaseRequested();

private:
    QLabel* m_subtitleLabel = nullptr;
    QPushButton* m_importButton = nullptr;
    QPushButton* m_openDbButton = nullptr;
    QPushButton* m_createDbButton = nullptr;
};
