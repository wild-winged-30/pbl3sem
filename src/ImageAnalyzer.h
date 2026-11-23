#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include "Defect.h"

struct Detection {
    DefectType type;
    double measure;  // see Defect
    cv::Rect box;    // in coordinates of the (resized) annotated image
};

class ImageAnalyzer {
public:
    // Returns detections and an annotated copy of the (resized) image.
    std::vector<Detection> analyze(const cv::Mat& bgr, cv::Mat& annotated) const;
};
