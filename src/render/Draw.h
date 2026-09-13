#pragma once

#include "raylib.h"
#include "../Sprite.h"

// Small shared helpers wrapping raylib's DrawTexturePro for the two draw shapes this game needs:
// a plain (optionally horizontally flipped) blit, and a shot spinning around its own center.
//
// Rotation note: raylib's DrawTexturePro rotates *clockwise* for a positive angle (it applies a
// standard rotation matrix in a y-down coordinate space), whereas the original libGDX port's
// SpriteBatch::draw rotates *counter-clockwise* for a positive angle (also a standard rotation
// matrix, but in y-up space). To reproduce the exact same on-screen spin direction, every angle
// coming from the original game logic (sh.rot * 90) must be negated before being handed to raylib.
namespace RDraw {

inline void sprite(const Sprite& s, float dstX, float dstY, float dstW, float dstH, bool flipX = false, Color tint = WHITE) {
    if (!s.valid()) return;
    Rectangle src{(float)s.x, (float)s.y, flipX ? -(float)s.w : (float)s.w, (float)s.h};
    Rectangle dst{dstX, dstY, dstW, dstH};
    DrawTexturePro(*s.tex, src, dst, {0, 0}, 0.0f, tint);
}

// Draws centered on (cx, cy), rotated by rotationDegCCW degrees counter-clockwise (matching the
// original game logic's convention - the clockwise conversion for raylib happens inside).
inline void spriteRotatedCentered(const Sprite& s, float cx, float cy, float w, float h, float rotationDegCCW, Color tint = WHITE) {
    if (!s.valid()) return;
    Rectangle src{(float)s.x, (float)s.y, (float)s.w, (float)s.h};
    Rectangle dst{cx, cy, w, h};
    DrawTexturePro(*s.tex, src, dst, {w / 2.0f, h / 2.0f}, -rotationDegCCW, tint);
}

} // namespace RDraw
