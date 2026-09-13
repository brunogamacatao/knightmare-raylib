#pragma once

#include "raylib.h"

// Lightweight stand-in for libGDX's TextureRegion: a sheet reference plus a pixel rect within it.
// Value type (just a pointer + 4 ints), safe to copy freely and to leave default-constructed
// (tex == nullptr) as the "no region assigned" state.
struct Sprite {
    const Texture2D* tex = nullptr;
    int x = 0, y = 0, w = 0, h = 0;

    bool valid() const { return tex != nullptr; }
};
