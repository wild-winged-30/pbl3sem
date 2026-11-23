#pragma once
// OOP model of a single road defect.
#include <algorithm>
#include <string>
#include <utility>

enum class DefectType { Pothole, Crack };
enum class Severity { Low = 1, Medium = 2, High = 3, Critical = 4 };

struct GeoPoint {
    double lat = 0.0;
    double lon = 0.0;
    bool valid = false;
};

inline const char* toString(DefectType t) { return t == DefectType::Pothole ? "Pothole" : "Crack"; }
inline const char* toString(Severity s) {
    switch (s) {
        case Severity::Low: return "Low";
        case Severity::Medium: return "Medium";
        case Severity::High: return "High";
        default: return "Critical";
    }
}

class Defect {
public:
    // measure: for potholes the fraction of the road area covered by the defect;
    //          for cracks the crack length as a fraction of the image width.
    Defect(int id, DefectType type, double measure, std::string image, GeoPoint loc)
        : id_(id), type_(type), measure_(measure), image_(std::move(image)), loc_(loc),
          severity_(computeSeverity(type, measure)) {}

    // Simple, predictable severity rule (see README).
    static Severity computeSeverity(DefectType type, double m) {
        if (type == DefectType::Pothole) {
            if (m < 0.01) return Severity::Low;
            if (m < 0.03) return Severity::Medium;
            if (m < 0.07) return Severity::High;
            return Severity::Critical;
        }
        if (m < 0.15) return Severity::Low;
        if (m < 0.30) return Severity::Medium;
        if (m < 0.50) return Severity::High;
        return Severity::Critical;
    }

    // Higher score = repair sooner. Severity dominates; size and type break ties.
    double priorityScore() const {
        double base = static_cast<int>(severity_) * 10.0;
        double size = std::min(measure_ * (type_ == DefectType::Pothole ? 50.0 : 10.0), 9.0);
        double typeBonus = (type_ == DefectType::Pothole) ? 2.0 : 0.0;
        return base + size + typeBonus;
    }

    int id() const { return id_; }
    DefectType type() const { return type_; }
    Severity severity() const { return severity_; }
    double measure() const { return measure_; }
    const std::string& image() const { return image_; }
    const GeoPoint& location() const { return loc_; }
    void setLocation(GeoPoint p) { loc_ = p; }

private:
    int id_;
    DefectType type_;
    double measure_;
    std::string image_;
    GeoPoint loc_;
    Severity severity_;
};
