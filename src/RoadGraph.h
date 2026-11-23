#pragma once
// Graph + Dijkstra shortest path + priority-aware route planning.
#include <utility>
#include <vector>
#include "Defect.h"

struct RoutePlan {
    std::vector<int> visitOrder;   // defect ids in the order they are visited
    std::vector<GeoPoint> path;    // full polyline following graph edges
    double totalKm = 0.0;
    bool empty() const { return visitOrder.empty(); }
};

double haversineKm(const GeoPoint& a, const GeoPoint& b);

class RoadGraph {
public:
    // Nodes are points; each connects to its k nearest neighbours and
    // components are joined so that the graph is always connected.
    void build(const std::vector<GeoPoint>& pts, int k = 3);
    // Dijkstra from src; returns distances (km) and fills parent[].
    std::vector<double> dijkstra(int src, std::vector<int>& parent) const;
    int size() const { return static_cast<int>(adj_.size()); }

private:
    void addEdge(int a, int b, double w);
    std::vector<std::vector<std::pair<int, double>>> adj_;
};

class RoutePlanner {
public:
    // ranked: defects sorted by priority (most urgent first). Only located defects are used.
    // Starts at the most urgent defect, then repeatedly goes to the unvisited defect with the
    // lowest (shortest-path distance / priority score), so urgent defects pull the route.
    static RoutePlan plan(const std::vector<const Defect*>& ranked);
};
