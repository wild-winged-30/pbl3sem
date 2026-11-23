#include "RoadGraph.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <numeric>
#include <queue>

double haversineKm(const GeoPoint& a, const GeoPoint& b) {
    const double R = 6371.0088, rad = M_PI / 180.0;
    double dLat = (b.lat - a.lat) * rad, dLon = (b.lon - a.lon) * rad;
    double h = std::sin(dLat / 2) * std::sin(dLat / 2) +
               std::cos(a.lat * rad) * std::cos(b.lat * rad) * std::sin(dLon / 2) * std::sin(dLon / 2);
    return 2 * R * std::asin(std::min(1.0, std::sqrt(h)));
}

void RoadGraph::addEdge(int a, int b, double w) {
    if (a == b) return;
    adj_[a].push_back({b, w});
    adj_[b].push_back({a, w});
}

void RoadGraph::build(const std::vector<GeoPoint>& pts, int k) {
    int n = static_cast<int>(pts.size());
    adj_.assign(n, {});
    if (n < 2) return;

    // k-nearest-neighbour edges
    for (int i = 0; i < n; ++i) {
        std::vector<std::pair<double, int>> d;
        for (int j = 0; j < n; ++j) if (j != i) d.push_back({haversineKm(pts[i], pts[j]), j});
        std::sort(d.begin(), d.end());
        for (int m = 0; m < k && m < static_cast<int>(d.size()); ++m) addEdge(i, d[m].second, d[m].first);
    }

    // Union-find: join components with the shortest bridging edge until connected.
    std::vector<int> parent(n);
    std::iota(parent.begin(), parent.end(), 0);
    std::function<int(int)> find = [&](int x) { return parent[x] == x ? x : parent[x] = find(parent[x]); };
    for (int i = 0; i < n; ++i)
        for (auto& e : adj_[i]) parent[find(i)] = find(e.first);

    while (true) {
        double best = std::numeric_limits<double>::infinity();
        int bi = -1, bj = -1;
        for (int i = 0; i < n; ++i)
            for (int j = i + 1; j < n; ++j)
                if (find(i) != find(j)) {
                    double w = haversineKm(pts[i], pts[j]);
                    if (w < best) { best = w; bi = i; bj = j; }
                }
        if (bi < 0) break;
        addEdge(bi, bj, best);
        parent[find(bi)] = find(bj);
    }
}

std::vector<double> RoadGraph::dijkstra(int src, std::vector<int>& parent) const {
    const double INF = std::numeric_limits<double>::infinity();
    int n = size();
    std::vector<double> dist(n, INF);
    parent.assign(n, -1);
    using P = std::pair<double, int>;
    std::priority_queue<P, std::vector<P>, std::greater<P>> pq;
    dist[src] = 0;
    pq.push({0.0, src});
    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (d > dist[u]) continue;
        for (auto& [v, w] : adj_[u]) {
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                parent[v] = u;
                pq.push({dist[v], v});
            }
        }
    }
    return dist;
}

RoutePlan RoutePlanner::plan(const std::vector<const Defect*>& ranked) {
    RoutePlan plan;
    std::vector<const Defect*> nodes;
    std::vector<GeoPoint> pts;
    for (auto* d : ranked)
        if (d->location().valid) { nodes.push_back(d); pts.push_back(d->location()); }
    if (nodes.empty()) return plan;

    RoadGraph g;
    g.build(pts, 3);

    int n = static_cast<int>(nodes.size());
    std::vector<bool> visited(n, false);
    int cur = 0;  // highest priority defect
    visited[0] = true;
    plan.visitOrder.push_back(nodes[0]->id());
    plan.path.push_back(pts[0]);

    for (int step = 1; step < n; ++step) {
        std::vector<int> parent;
        std::vector<double> dist = g.dijkstra(cur, parent);
        int next = -1;
        double bestCost = std::numeric_limits<double>::infinity();
        for (int j = 0; j < n; ++j) {
            if (visited[j]) continue;
            double cost = dist[j] / nodes[j]->priorityScore();
            if (cost < bestCost) { bestCost = cost; next = j; }
        }
        if (next < 0) break;

        // Rebuild the node path cur -> next from the parent array.
        std::vector<int> seg;
        for (int v = next; v != -1 && v != cur; v = parent[v]) seg.push_back(v);
        std::reverse(seg.begin(), seg.end());
        for (int v : seg) plan.path.push_back(pts[v]);

        plan.totalKm += dist[next];
        visited[next] = true;
        plan.visitOrder.push_back(nodes[next]->id());
        cur = next;
    }
    return plan;
}
