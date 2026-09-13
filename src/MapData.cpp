#include "MapData.h"

#include <stdexcept>
#include "raylib.h"
#include "Constants.h"

// Loaded via raylib's LoadFileData() rather than plain stdio: on Android/iOS builds this
// transparently redirects through the platform's bundled-asset APIs (APK assets / app bundle
// resources) instead of the regular filesystem, which is what actually works once packaged.
MapData::MapData(const std::string& path) {
    int bytesRead = 0;
    unsigned char* data = LoadFileData(path.c_str(), &bytesRead);
    if (!data) throw std::runtime_error("Failed to open map file: " + path);

    size_t count = static_cast<size_t>(bytesRead) / 2;
    tiles.resize(count);
    for (size_t i = 0; i < count; i++) {
        int hi = data[i * 2];
        int lo = data[i * 2 + 1];
        tiles[i] = (hi << 8) | lo;
    }
    UnloadFileData(data);
}

int MapData::rawAt(int row, int col) const {
    int idx = row * Constants::MAP_COLS + col;
    if (idx < 0 || idx >= static_cast<int>(tiles.size())) return 0;
    return tiles[idx];
}

int MapData::tileId(int row, int col) const { return rawAt(row, col) & 0x7F; }

bool MapData::isSolid(int row, int col) const { return (rawAt(row, col) & 0x80) != 0; }

int MapData::objectId(int row, int col) const { return (rawAt(row, col) >> 8) & 0x1F; }

int MapData::objectExtra(int row, int col) const { return (rawAt(row, col) >> 13) & 0x7; }

void MapData::setSolid(int row, int col, bool solid) {
    int idx = row * Constants::MAP_COLS + col;
    if (idx < 0 || idx >= static_cast<int>(tiles.size())) return;
    if (solid) tiles[idx] |= 0x80; else tiles[idx] &= 0xFF7F;
}

MapData MapData::copy() const {
    MapData m;
    m.tiles = tiles;
    return m;
}
