#pragma once
#include <QString>
#include <QStringList>

class ImageManager {
public:
    static bool analyze(const QString &path, double &brightness, double &edgePercent, QString &error);
};
