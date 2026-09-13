#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Loads and stores the binary map format produced by map_converter.js:
// 2 bytes per tile, big endian.
// bits 0-6:  tile id (7 bits)
// bit  7:    solid flag
// bits 8-12: object/enemy/power-up id (5 bits)
// bits 13-15: object properties (3 bits)
class MapData {
public:
    std::vector<int> tiles;

    explicit MapData(const std::string& path);

    int rawAt(int row, int col) const;
    int tileId(int row, int col) const;
    bool isSolid(int row, int col) const;
    int objectId(int row, int col) const;
    int objectExtra(int row, int col) const;
    void setSolid(int row, int col, bool solid);

    // Stages are cached/shared; each play-through needs its own mutable copy (blocks alter the solid bits).
    MapData copy() const;

private:
    MapData() = default;
};
