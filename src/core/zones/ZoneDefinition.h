#pragma once

#include <QColor>
#include <QString>

enum class ColorMetric
{
    None,
    Power,
    HeartRate,
    Cadence,
    Speed,
    Altitude,
    Gradient
};

struct Zone
{
    QString name;
    double minValue = 0.0;
    double maxValue = 0.0;
    QColor color;
};

struct ZoneDistribution
{
    Zone zone;
    double seconds = 0.0;
    double percent = 0.0;
};

struct ColorContext
{
    int ftp = 250;
    int heartRateMax = 190;
    int heartRateRest = 60;
    int thresholdHeartRate = 0;
    double altitudeMin = 0.0;
    double altitudeMax = 0.0;
    bool hasAltitudeRange = false;
};

QString colorMetricDisplayName(ColorMetric metric);
QString colorMetricUnit(ColorMetric metric);