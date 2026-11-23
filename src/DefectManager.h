#pragma once
// Stores defects and ranks them for repair with a priority queue (max-heap).
#include <queue>
#include <vector>
#include "Defect.h"

struct ByPriority {
    bool operator()(const Defect* a, const Defect* b) const {
        if (a->priorityScore() != b->priorityScore()) return a->priorityScore() < b->priorityScore();
        return a->id() > b->id();  // older defect first on ties
    }
};

class DefectManager {
public:
    Defect& add(DefectType t, double measure, const std::string& image, GeoPoint loc) {
        defects_.emplace_back(nextId_++, t, measure, image, loc);
        return defects_.back();
    }

    const std::vector<Defect>& all() const { return defects_; }

    // Pops a priority queue until empty -> list sorted from most to least urgent.
    std::vector<const Defect*> ranked() const {
        std::priority_queue<const Defect*, std::vector<const Defect*>, ByPriority> pq;
        for (const auto& d : defects_) pq.push(&d);
        std::vector<const Defect*> out;
        out.reserve(defects_.size());
        while (!pq.empty()) { out.push_back(pq.top()); pq.pop(); }
        return out;
    }

    // Defects with no reliable coordinates are flagged, never placed at a guessed spot.
    std::vector<const Defect*> unlocated() const {
        std::vector<const Defect*> out;
        for (const auto& d : defects_) if (!d.location().valid) out.push_back(&d);
        return out;
    }

    void setLocationForImage(const std::string& image, GeoPoint p) {
        for (auto& d : defects_) if (d.image() == image) d.setLocation(p);
    }

    void clear() { defects_.clear(); nextId_ = 1; }
    bool empty() const { return defects_.empty(); }

private:
    std::vector<Defect> defects_;
    int nextId_ = 1;
};
