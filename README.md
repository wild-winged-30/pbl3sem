<<<<<<< HEAD
# SmartRoad: Image-Based Road Condition Monitoring, Mapping and Repair Prioritization

Desktop prototype (C++17, GTK3, OpenCV) that finds potholes and cracks in road photos,
places them on a Leaflet/OpenStreetMap map, ranks them for repair and plans a visiting route.
Team TradeSim, DSCPP-III-2026-T264, Graphic Era University.

## Build (Ubuntu / Debian)
```bash
sudo apt install build-essential cmake pkg-config libopencv-dev libgtk-3-dev
cmake -S . -B build
cmake --build build -j
./build/smartroad                 # GUI
./build/smartroad --cli samples/synthetic_road.jpg photo2.jpg   # headless test
```
The map page needs internet for the Leaflet library and OpenStreetMap tiles.

## Using the GUI
1. **Add images** (JPEG photos from a phone keep their GPS tag), then **Analyze**.
2. The table lists defects by repair priority; select a row to see the annotated photo.
3. Photos without GPS are flagged. Select one and use **Set location** to type coordinates.
4. **Plan route** computes a visit order; **Open map** shows markers (colour = severity) and the route.
5. **Export CSV** saves the ranked report.

## Architecture (matches the Phase-III report)
| Component | Files | Concepts |
|---|---|---|
| Image processing | `ImageAnalyzer.*` | OpenCV: CLAHE, blur, threshold, morphology, contours, black-hat |
| Defect management | `Defect.h`, `DefectManager.h` | OOP classes, **priority queue (max-heap)** ranking |
| Location | `ExifReader.*` | Own EXIF/GPS parser, missing GPS flagged, never guessed |
| Map + route | `RoadGraph.*`, `MapExporter.*`, `web/map.html` | **Graph, Dijkstra**, Leaflet + OpenStreetMap |
| GUI / glue | `gui.cpp`, `Pipeline.*`, `main.cpp` | GTK3 |

Data flow: GTK -> `processImage` -> `Defect` records -> `DefectManager` ranking -> `RoutePlanner`
-> `web/data.js` (shared defect-record format) -> `web/map.html`.

## Detection, severity and priority rules
* Only the lower 65% of the photo is treated as road.
* **Pothole**: dark compact blob (solidity >= 0.6, aspect <= 4, 0.4%-45% of road area).
  Severity by area share: <1% Low, <3% Medium, <7% High, else Critical.
* **Crack**: thin elongated dark structure (aspect >= 3.5, length >= 8% of image width).
  Severity by length share of image width: <15% Low, <30% Medium, <50% High, else Critical.
* Priority score = 10 x severity level + size term (max 9) + 2 for potholes. Highest score first.
* Route: graph of defects (3 nearest neighbours, components bridged so it is connected, haversine km).
  Start at the top-priority defect, then go to the unvisited defect with the lowest
  `shortest-path distance / priority score`, expanding each leg with Dijkstra.

## Limitations / future work
Classical image processing is sensitive to lighting and texture; a trained detector (e.g. YOLO) would be
more accurate. Routing uses straight-line graph edges, not real road data. Live GPS capture is not included.
=======
# pbl3sem
>>>>>>> eb0e81fe2205d227f7b6484f21e957609beb3727
