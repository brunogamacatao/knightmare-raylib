#pragma once

#include <string>
#include "raylib.h"
#include "../Assets.h"

// Port of print.inc: draws text using the original 8x8 bitmap font baked into tiles/objects.png.
class Hud {
public:
    explicit Hud(const Assets& assets) : assets_(assets) {}

    // Draws text at (x,y) top-left, each glyph tileSize wide/tall (raylib convention: y grows downward).
    void draw(const std::string& text, float x, float y, float tileSize, Color tint = WHITE) const;

    float width(const std::string& text, float tileSize) const { return text.size() * tileSize; }

private:
    const Assets& assets_;
    Sprite glyph(char ch) const;
};
