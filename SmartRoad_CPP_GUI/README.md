# SmartRoad GUI — C++ / Qt Widgets / OpenCV

This is a desktop GUI version of SmartRoad. It does not use Python. The interface includes Dashboard, Add Report, View/Search Reports, and Analytics & Map tabs. Location is entered as a place/address; latitude and longitude are not required. The OpenStreetMap button opens a browser search for that place.

## Modules
- **OOP:** `RoadReport` data model, `ReportManager`, `ImageManager`, and `SmartRoadWindow` classes.
- **DSA:** custom singly linked list for reports, FIFO queue for pending reports, LIFO stack for most recent report ID.
- **OpenCV:** reads road photos, calculates average grayscale brightness and Canny edge-pixel percentage. These statistics are not pothole detection.
- **Map:** opens the entered place in OpenStreetMap search in the default browser. This starter GUI does not geocode all reports or draw a custom marker layer.
- **Persistence:** reports are saved to `data/reports.json` in the current working directory.

## Install dependencies (Ubuntu WSL)
```bash
sudo apt update
sudo apt install -y build-essential cmake qtbase5-dev libopencv-dev
```

## Build and run
From the extracted `SmartRoad_CPP_GUI` directory:
```bash
cmake -S . -B build
cmake --build build -j2
./build/smartroad_gui
```

Keep the terminal in this folder when launching so `data/reports.json` is saved there. On Windows 11 with WSLg, Qt GUI windows generally work. If the window cannot open, check WSLg/GUI support.

## Honest testing status
Source package was generated, but this environment did not perform a full build against the user's Ubuntu 26.04 WSL dependencies or test the GUI interactively. Build errors, if any, should be fixed based on the actual compiler output. OpenCV analysis is basic image statistics and must not be represented as automatic pothole classification.
