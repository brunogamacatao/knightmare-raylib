#pragma once

#include <cmath>
#include "../Sprite.h"

// Direct port of one slot of the g_obj() array: 0:id 1:x 2:y 3:gpr1(life) 4:gpr2 5:gpr3 6:shadow ref.
// Also carries the rendering state that in the original lives in the hardware sprite engine.
struct GameObj {
    int id = 0; // 0 = free slot
    float x = 0, y = 0;
    float gpr1 = 0; // life / generic
    float gpr2 = 0;
    float gpr3 = 0;
    // Bidirectional link: on an owner, its shadow; on a shadow (id 39), its owner. nullptr = none.
    // Non-owning: entities are owned by GameState's vectors of unique_ptr.
    GameObj* shadowRef = nullptr;

    // Rendering
    Sprite region;
    int rot = 0;          // rotation in 90 degree steps (0..3), used by shot-like objects (unused on GameObj itself)
    bool flipX = false, flipY = false;
    float renderOffsetX = 0, renderOffsetY = 0;
    int layer = 1;
    bool hidden = false;
    // Custom collision box size for objects with no OBJ_TILE entry (invisible hit boxes, id 60). -1 = use OBJ_TILE size.
    float hitW = -1, hitH = -1;
    // Extra per-instance slot reused by a few bosses (e.g. stage 5's helmet open/closed flag).
    float aux1 = -1;

    // Previous frame position, used to emulate MMBasic's SPRITE(V) movement-angle query.
    float prevX = 0, prevY = 0;
    bool hasPrev = false;

    bool isFree() const { return id == 0; }

    void free() {
        id = 0; x = 0; y = 0; gpr1 = 0; gpr2 = 0; gpr3 = 0; shadowRef = nullptr;
        region = Sprite{}; hidden = true; hasPrev = false;
        flipX = false; flipY = false; renderOffsetX = 0; renderOffsetY = 0; layer = 1;
        hitW = -1; hitH = -1; aux1 = -1;
    }

    // Compass-style angle (0 = up, clockwise) of the last movement vector, matching
    // deg(sprite(V, id, 1)) as used by objects.inc/boss.inc (angle 0=up, 90=right, 180=down, 270=left).
    // Normalized to [0, 360).
    float velocityAngleDeg() const {
        if (!hasPrev) return 0;
        float dx = x - prevX, dy = y - prevY;
        if (dx == 0 && dy == 0) return 0;
        float a = std::atan2(dx, -dy) * 180.0f / (float)M_PI;
        if (a < 0) a += 360;
        return a;
    }

    void updatePrev() {
        prevX = x; prevY = y; hasPrev = true;
    }
};
