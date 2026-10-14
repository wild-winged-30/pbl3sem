#pragma once
#include <QString>
#include <QDateTime>

struct RoadReport {
    int id = 0;
    QString place;
    QString severity;
    QString status;
    QString description;
    QString imagePath;
    QDateTime createdAt;
    double brightness = -1.0;
    double edgePercent = -1.0;
};
