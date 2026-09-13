#pragma once

#include <array>
#include "GameObj.h"

// Direct port of the g_boss() array: 0:status 1:body 2..7:hitBox. Cross-references are held as
// non-owning raw pointers to entities owned by GameState (not array indices), so they stay valid
// regardless of how the backing object list is compacted.
struct BossState {
    static constexpr int NOT_SPAWNED = 0;
    static constexpr int WAITING = 1;
    static constexpr int ACTIVE = 2; // and above

    int status = 0;
    GameObj* body = nullptr;
    std::array<GameObj*, 8> hitBox{}; // indices 2..7 used

    void reset() {
        status = 0;
        body = nullptr;
        hitBox.fill(nullptr);
    }
};
