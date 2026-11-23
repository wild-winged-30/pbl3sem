#include "ExifReader.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

namespace {

void parseExif(const unsigned char* t, size_t size, GeoPoint& g) {
    if (size < 8) return;
    bool le;
    if (t[0] == 'I' && t[1] == 'I') le = true;
    else if (t[0] == 'M' && t[1] == 'M') le = false;
    else return;

    auto u16 = [&](size_t o) -> uint32_t {
        if (o + 2 > size) return 0;
        return le ? (t[o] | (t[o + 1] << 8)) : ((t[o] << 8) | t[o + 1]);
    };
    auto u32 = [&](size_t o) -> uint32_t {
        if (o + 4 > size) return 0;
        if (le) return (uint32_t)t[o] | ((uint32_t)t[o + 1] << 8) | ((uint32_t)t[o + 2] << 16) | ((uint32_t)t[o + 3] << 24);
        return ((uint32_t)t[o] << 24) | ((uint32_t)t[o + 1] << 16) | ((uint32_t)t[o + 2] << 8) | (uint32_t)t[o + 3];
    };
    auto rational = [&](size_t o) -> double {
        uint32_t num = u32(o), den = u32(o + 4);
        return den ? static_cast<double>(num) / den : 0.0;
    };
    auto dms = [&](size_t o, double& out) {
        out = rational(o) + rational(o + 8) / 60.0 + rational(o + 16) / 3600.0;
    };

    uint32_t ifd0 = u32(4);
    if (ifd0 + 2 > size) return;
    uint32_t cnt = u16(ifd0);
    uint32_t gpsOff = 0;
    for (uint32_t i = 0; i < cnt; ++i) {
        size_t e = ifd0 + 2 + i * 12;
        if (e + 12 > size) return;
        if (u16(e) == 0x8825) gpsOff = u32(e + 8);
    }
    if (!gpsOff || gpsOff + 2 > size) return;

    cnt = u16(gpsOff);
    char latRef = 0, lonRef = 0;
    double lat = 0, lon = 0;
    bool haveLat = false, haveLon = false;
    for (uint32_t i = 0; i < cnt; ++i) {
        size_t e = gpsOff + 2 + i * 12;
        if (e + 12 > size) return;
        uint32_t tag = u16(e), type = u16(e + 2), count = u32(e + 4), val = u32(e + 8);
        if (tag == 1) latRef = static_cast<char>(t[e + 8]);
        else if (tag == 3) lonRef = static_cast<char>(t[e + 8]);
        else if ((tag == 2 || tag == 4) && type == 5 && count == 3 && val + 24 <= size) {
            if (tag == 2) { dms(val, lat); haveLat = true; }
            else { dms(val, lon); haveLon = true; }
        }
    }
    if (!haveLat || !haveLon) return;
    if (latRef == 'S') lat = -lat;
    if (lonRef == 'W') lon = -lon;
    if (lat < -90 || lat > 90 || lon < -180 || lon > 180) return;
    if (lat == 0.0 && lon == 0.0) return;  // empty GPS block
    g.lat = lat;
    g.lon = lon;
    g.valid = true;
}

}  // namespace

GeoPoint readGpsFromJpeg(const std::string& path) {
    GeoPoint g;
    std::ifstream f(path, std::ios::binary);
    if (!f) return g;
    std::vector<unsigned char> d(262144);
    f.read(reinterpret_cast<char*>(d.data()), static_cast<std::streamsize>(d.size()));
    size_t n = static_cast<size_t>(f.gcount());
    if (n < 4 || d[0] != 0xFF || d[1] != 0xD8) return g;  // not a JPEG

    size_t pos = 2;
    while (pos + 4 <= n) {
        if (d[pos] != 0xFF) break;
        unsigned char marker = d[pos + 1];
        if (marker == 0xFF) { ++pos; continue; }
        if (marker == 0xDA) break;  // start of scan: no more metadata
        size_t len = (static_cast<size_t>(d[pos + 2]) << 8) | d[pos + 3];
        if (len < 2) break;
        if (marker == 0xE1 && pos + 10 <= n && std::memcmp(&d[pos + 4], "Exif\0\0", 6) == 0) {
            size_t avail = std::min(n - (pos + 10), len > 8 ? len - 8 : size_t(0));
            parseExif(&d[pos + 10], avail, g);
            return g;
        }
        pos += 2 + len;
    }
    return g;
}
