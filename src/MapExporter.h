#pragma once
#include <string>
#include <vector>
#include "Defect.h"
#include "RoadGraph.h"

// Writes data.js (window.SMARTROAD_DATA = {...}) that web/map.html loads.
bool exportMapData(const std::string& path, const std::vector<const Defect*>& ranked, const RoutePlan* route);
// Writes a CSV report of all defects in priority order.
bool exportCsv(const std::string& path, const std::vector<const Defect*>& ranked);
