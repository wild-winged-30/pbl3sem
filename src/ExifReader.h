#pragma once
#include <string>
#include "Defect.h"

// Reads GPS latitude/longitude from a JPEG's EXIF block (no external library).
// Returns valid=false when the file has no usable GPS data.
GeoPoint readGpsFromJpeg(const std::string& path);
