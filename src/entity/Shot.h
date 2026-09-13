#pragma once

#include "../Sprite.h"

// Direct port of one slot of the g_shots() array: 0:weaponId 1:x 2:y 3:gpr1 4:gpr2 5:gpr3.
struct Shot {
    int weaponId = 0; // 0 = free
    float x = 0, y = 0;
    float gpr1 = 0, gpr2 = 0, gpr3 = 0;

    Sprite region;
    int rot = 0;
    int tileId = 0;
    int offsetX = 0;
    // Stable creation-order id, used only to alternate cosmetic behaviour (e.g. spin direction)
    // between shots without depending on their transient position in a list.
    int seq = 0;

    bool isFree() const { return weaponId == 0; }

    void free() {
        weaponId = 0; x = 0; y = 0; gpr1 = 0; gpr2 = 0; gpr3 = 0; region = Sprite{};
    }
};

inline int nextShotSeq() {
    static int next = 0;
    return next++;
}
