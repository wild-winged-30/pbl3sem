#include <cstdio>
#include <cstring>
#include <filesystem>
#include "DefectManager.h"
#include "MapExporter.h"
#include "Pipeline.h"
#include "RoadGraph.h"
#include "gui.h"

#ifndef SMARTROAD_WEB_DIR
#define SMARTROAD_WEB_DIR "web"
#endif

// Headless mode: smartroad --cli img1.jpg img2.jpg ...
static int runCli(int argc, char** argv) {
    DefectManager mgr;
    for (int i = 2; i < argc; ++i) {
        cv::Mat annotated;
        std::string err;
        int n = processImage(argv[i], mgr, annotated, err);
        if (n < 0) { std::fprintf(stderr, "%s\n", err.c_str()); continue; }
        std::string out = "annotated_" + std::filesystem::path(argv[i]).filename().string();
        cv::imwrite(out, annotated);
        std::printf("%s: %d defect(s) -> %s\n", argv[i], n, out.c_str());
    }
    auto ranked = mgr.ranked();
    std::printf("\nRank  Type     Severity  Score  Location\n");
    int r = 1;
    for (auto* d : ranked) {
        char loc[64] = "no GPS (flagged)";
        if (d->location().valid) std::snprintf(loc, sizeof loc, "%.5f, %.5f", d->location().lat, d->location().lon);
        std::printf("%-5d %-8s %-9s %-6.1f %s\n", r++, toString(d->type()), toString(d->severity()),
                    d->priorityScore(), loc);
    }
    RoutePlan route = RoutePlanner::plan(ranked);
    if (!route.empty()) {
        std::printf("\nRoute (defect ids):");
        for (int id : route.visitOrder) std::printf(" %d", id);
        std::printf("\nTotal distance: %.2f km\n", route.totalKm);
    }
    std::string data = std::string(SMARTROAD_WEB_DIR) + "/data.js";
    if (exportMapData(data, ranked, route.empty() ? nullptr : &route))
        std::printf("\nMap data written to %s (open web/map.html)\n", data.c_str());
    return 0;
}

int main(int argc, char** argv) {
    if (argc > 1 && std::strcmp(argv[1], "--cli") == 0) return runCli(argc, argv);
    return runGui(argc, argv);
}
