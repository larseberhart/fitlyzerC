// SPDX-License-Identifier: GPL-3

/**
 * @file fitlyzerC.cpp
 * @brief Application entry point and UI bootstrap.
 *
 * Initializes the Qt application, configures style and metadata, and launches the main window event loop.
 *
 * Responsibilities:
 * - Initialize the application and start the main UI event loop
 *
 * @author Lars EBERHART
 */

#include <QApplication>
#include <QIcon>
#include <QPalette>
#include <QStyleFactory>

#include "gui/MainWindow.h"

/**
 * @brief Application entry point.
 *
 * Initializes Qt, application settings, database connections,
 * and launches the main window with configured styling.
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("Fitlyzer");
    app.setApplicationName("FitlyzerC");
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(245, 247, 250));
    lightPalette.setColor(QPalette::WindowText, QColor(31, 41, 55));
    lightPalette.setColor(QPalette::Base, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::AlternateBase, QColor(241, 245, 249));
    lightPalette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::ToolTipText, QColor(31, 41, 55));
    lightPalette.setColor(QPalette::Text, QColor(31, 41, 55));
    lightPalette.setColor(QPalette::Button, QColor(239, 243, 248));
    lightPalette.setColor(QPalette::ButtonText, QColor(31, 41, 55));
    lightPalette.setColor(QPalette::BrightText, Qt::white);
    lightPalette.setColor(QPalette::Highlight, QColor(37, 99, 235));
    lightPalette.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(lightPalette);

    app.setWindowIcon(QIcon(":/resources/icons/fitlyzer_logo.png"));

    MainWindow window;
    window.show();

    return app.exec();
}