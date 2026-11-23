cmake_minimum_required(VERSION 3.12)
project(SmartRoad VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

add_executable(smartroad
  src/main.cpp
  src/gui.cpp
  src/Pipeline.cpp
  src/ImageAnalyzer.cpp
  src/ExifReader.cpp
  src/RoadGraph.cpp
  src/MapExporter.cpp
)

target_include_directories(smartroad PRIVATE src ${OpenCV_INCLUDE_DIRS} ${GTK3_INCLUDE_DIRS})
target_link_directories(smartroad PRIVATE ${GTK3_LIBRARY_DIRS})
target_link_libraries(smartroad PRIVATE ${OpenCV_LIBS} ${GTK3_LIBRARIES})
target_compile_options(smartroad PRIVATE ${GTK3_CFLAGS_OTHER} -Wall -Wextra)

# Folder that holds map.html; data.js is written here so the map can load it.
target_compile_definitions(smartroad PRIVATE SMARTROAD_WEB_DIR="${CMAKE_SOURCE_DIR}/web")
