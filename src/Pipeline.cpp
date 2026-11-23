#include "Pipeline.h"
#include "ExifReader.h"
#include "ImageAnalyzer.h"

int processImage(const std::string& path, DefectManager& mgr, cv::Mat& annotated, std::string& err) {
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    if (img.empty()) {
        err = "Could not read image: " + path;
        return -1;
    }
    GeoPoint gps = readGpsFromJpeg(path);  // valid=false if the photo has no GPS tag
    ImageAnalyzer analyzer;
    auto detections = analyzer.analyze(img, annotated);
    for (const auto& d : detections) mgr.add(d.type, d.measure, path, gps);
    return static_cast<int>(detections.size());
}
