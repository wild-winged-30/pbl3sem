#include "ImageManager.h"
#include <opencv2/opencv.hpp>

bool ImageManager::analyze(const QString &path, double &brightness, double &edgePercent, QString &error) {
    cv::Mat image = cv::imread(path.toStdString());
    if (image.empty()) { error = "OpenCV could not read this image. Try JPG or PNG."; return false; }
    cv::Mat gray, edges;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    brightness = cv::mean(gray)[0];
    cv::Canny(gray, edges, 80, 160);
    const double edgePixels = static_cast<double>(cv::countNonZero(edges));
    edgePercent = 100.0 * edgePixels / static_cast<double>(edges.rows * edges.cols);
    return true;
}
