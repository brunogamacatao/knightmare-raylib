#pragma once

#include "../Sprite.h"

// Direct port of one slot of the g_blocks() array: 0:type 1:hits 2:row 3:col.
struct Block {
    // Types
    static constexpr int ROOK = 1;   // points
    static constexpr int KNIGHT = 2; // kill all enemies
    static constexpr int QUEEN = 3;  // extra life
    static constexpr int KING = 4;   // freeze
    static constexpr int BARRIER = 5;
    static constexpr int BRIDGE = 7;
    static constexpr int COLLECTED = 6;

    int type = 0; // 0 = free slot
    int hits = 0;
    int row = 0, col = 0;
    float x = 0, y = 0; // current sprite position (scrolls with the map)
    Sprite region;
    bool hidden = false;

    bool isFree() const { return type == 0; }

    void free() {
        type = 0; hits = 0; row = 0; col = 0; hidden = true;
    }
};
