#include "MapExporter.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {
std::string jsonEscape(const std::string& s) {
    std::string o;
    for (unsigned char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if (c < 0x20) { char b[8]; std::snprintf(b, sizeof b, "\\u%04x", c); o += b; }
                else o += static_cast<char>(c);
        }
    }
    return o;
}
std::string base(const std::string& p) { return std::filesystem::path(p).filename().string(); }
}  // namespace

bool exportMapData(const std::string& path, const std::vector<const Defect*>& ranked, const RoutePlan* route) {
    std::ofstream f(path);
    if (!f) return false;
    f << std::fixed << std::setprecision(6);
    f << "window.SMARTROAD_DATA = {\n  \"defects\": [\n";
    int rank = 1;
    for (size_t i = 0; i < ranked.size(); ++i) {
        const Defect* d = ranked[i];
        f << "    {\"rank\":" << rank++ << ",\"id\":" << d->id() << ",\"type\":\"" << toString(d->type())
          << "\",\"severity\":\"" << toString(d->severity()) << "\",\"level\":" << static_cast<int>(d->severity())
          << ",\"score\":" << std::setprecision(1) << d->priorityScore() << std::setprecision(6)
          << ",\"measure\":" << d->measure() << ",\"image\":\"" << jsonEscape(base(d->image())) << "\""
          << ",\"located\":" << (d->location().valid ? "true" : "false");
        if (d->location().valid) f << ",\"lat\":" << d->location().lat << ",\"lon\":" << d->location().lon;
        f << "}" << (i + 1 < ranked.size() ? "," : "") << "\n";
    }
    f << "  ],\n  \"route\": ";
    if (route && !route->empty()) {
        f << "{\"totalKm\":" << std::setprecision(3) << route->totalKm << std::setprecision(6) << ",\"order\":[";
        for (size_t i = 0; i < route->visitOrder.size(); ++i) f << (i ? "," : "") << route->visitOrder[i];
        f << "],\"path\":[";
        for (size_t i = 0; i < route->path.size(); ++i)
            f << (i ? "," : "") << "[" << route->path[i].lat << "," << route->path[i].lon << "]";
        f << "]}";
    } else {
        f << "null";
    }
    f << "\n};\n";
    return static_cast<bool>(f);
}

bool exportCsv(const std::string& path, const std::vector<const Defect*>& ranked) {
    std::ofstream f(path);
    if (!f) return false;
    f << "rank,id,type,severity,priority_score,measure,latitude,longitude,image\n";
    int rank = 1;
    for (const Defect* d : ranked) {
        f << rank++ << ',' << d->id() << ',' << toString(d->type()) << ',' << toString(d->severity()) << ','
          << std::fixed << std::setprecision(2) << d->priorityScore() << ',' << std::setprecision(4) << d->measure()
          << ',';
        if (d->location().valid) f << std::setprecision(6) << d->location().lat << ',' << d->location().lon;
        else f << ",";
        f << ",\"" << base(d->image()) << "\"\n";
    }
    return static_cast<bool>(f);
}
