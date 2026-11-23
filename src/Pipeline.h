#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include "DefectManager.h"

// Full per-image pipeline: read -> detect -> classify/severity -> GPS from EXIF -> store.
// Returns number of defects found, or -1 on error (err filled).
int processImage(const std::string& path, DefectManager& mgr, cv::Mat& annotated, std::string& err);
