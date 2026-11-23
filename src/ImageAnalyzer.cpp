#include "ImageAnalyzer.h"
#include <algorithm>
#include <cstdio>

using namespace cv;

namespace {
struct Cand {
    double measure;
    Rect box;
};
}

std::vector<Detection> ImageAnalyzer::analyze(const Mat& input, Mat& annotated) const {
    std::vector<Detection> out;
    if (input.empty()) return out;

    // 1. Normalise size, then grayscale + contrast + denoise.
    Mat img = input;
    const int maxW = 800;
    if (img.cols > maxW) {
        double s = static_cast<double>(maxW) / img.cols;
        resize(img, img, Size(), s, s, INTER_AREA);
    }
    annotated = img.clone();

    Mat gray;
    cvtColor(img, gray, COLOR_BGR2GRAY);
    Ptr<CLAHE> clahe = createCLAHE(2.0, Size(8, 8));
    clahe->apply(gray, gray);
    GaussianBlur(gray, gray, Size(5, 5), 0);

    // 2. Only the lower part of the photo is treated as road surface (skip sky/horizon).
    int top = static_cast<int>(img.rows * 0.35);
    Rect roiRect(0, top, img.cols, img.rows - top);
    Mat roi = gray(roiRect);
    double roiArea = static_cast<double>(roi.total());

    // 3. Potholes: large, compact, darker-than-surface blobs.
    Scalar mean, sd;
    meanStdDev(roi, mean, sd);
    Mat dark;
    threshold(roi, dark, mean[0] - 0.9 * sd[0], 255, THRESH_BINARY_INV);
    Mat k9 = getStructuringElement(MORPH_ELLIPSE, Size(9, 9));
    morphologyEx(dark, dark, MORPH_OPEN, k9);   // removes thin lines (cracks)
    morphologyEx(dark, dark, MORPH_CLOSE, k9);  // fills small holes

    std::vector<std::vector<Point>> contours;
    findContours(dark.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    std::vector<Cand> potholes;
    for (auto& c : contours) {
        double area = contourArea(c);
        double ratio = area / roiArea;
        if (ratio < 0.004 || ratio > 0.45) continue;
        std::vector<Point> hull;
        convexHull(c, hull);
        double hullArea = contourArea(hull);
        if (hullArea <= 0 || area / hullArea < 0.6) continue;  // ragged shapes are not potholes
        RotatedRect rr = minAreaRect(c);
        double L = std::max(rr.size.width, rr.size.height);
        double W = std::max(1.0f, std::min(rr.size.width, rr.size.height));
        if (W < 6 || L / W > 4.0) continue;
        Rect b = boundingRect(c);
        b.y += top;
        potholes.push_back({ratio, b});
    }
    std::sort(potholes.begin(), potholes.end(), [](const Cand& a, const Cand& b) { return a.measure > b.measure; });
    if (potholes.size() > 6) potholes.resize(6);

    for (auto& p : potholes) {
        out.push_back({DefectType::Pothole, p.measure, p.box});
        rectangle(annotated, p.box, Scalar(40, 40, 230), 2);
        char label[48];
        std::snprintf(label, sizeof label, "Pothole %.1f%%", p.measure * 100.0);
        putText(annotated, label, Point(p.box.x, std::max(14, p.box.y - 6)), FONT_HERSHEY_SIMPLEX, 0.5,
                Scalar(40, 40, 230), 2);
    }

    // 4. Cracks: thin, dark, elongated structures found with a black-hat filter.
    Mat bh;
    morphologyEx(roi, bh, MORPH_BLACKHAT, getStructuringElement(MORPH_ELLIPSE, Size(17, 17)));
    Mat tmp, crackMask;
    double otsu = threshold(bh, tmp, 0, 255, THRESH_BINARY | THRESH_OTSU);
    threshold(bh, crackMask, std::max(otsu, 18.0), 255, THRESH_BINARY);

    Mat potholeZone;
    dilate(dark, potholeZone, k9);
    crackMask.setTo(0, potholeZone);  // do not double-count pothole edges
    morphologyEx(crackMask, crackMask, MORPH_CLOSE, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

    contours.clear();
    findContours(crackMask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    std::vector<Cand> cracks;
    for (auto& c : contours) {
        if (c.size() < 5) continue;
        RotatedRect rr = minAreaRect(c);
        double L = std::max(rr.size.width, rr.size.height);
        double W = std::max(1.0f, std::min(rr.size.width, rr.size.height));
        double lenRatio = L / img.cols;
        if (lenRatio < 0.08 || L / W < 3.5) continue;
        Rect b = boundingRect(c);
        b.y += top;
        cracks.push_back({std::min(lenRatio, 1.0), b});
    }
    std::sort(cracks.begin(), cracks.end(), [](const Cand& a, const Cand& b) { return a.measure > b.measure; });
    if (cracks.size() > 6) cracks.resize(6);

    for (auto& c : cracks) {
        out.push_back({DefectType::Crack, c.measure, c.box});
        rectangle(annotated, c.box, Scalar(0, 190, 255), 2);
        char label[48];
        std::snprintf(label, sizeof label, "Crack %.0f%%w", c.measure * 100.0);
        putText(annotated, label, Point(c.box.x, std::max(14, c.box.y - 6)), FONT_HERSHEY_SIMPLEX, 0.5,
                Scalar(0, 190, 255), 2);
    }
    return out;
}
