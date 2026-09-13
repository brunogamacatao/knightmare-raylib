#include "Hud.h"

#include "Draw.h"
#include "../Constants.h"

void Hud::draw(const std::string& text, float x, float y, float tileSize, Color tint) const {
    float col = x;
    for (char ch : text) {
        Sprite r = glyph(ch);
        if (r.valid()) RDraw::sprite(r, col, y, tileSize, tileSize, false, tint);
        col += tileSize;
    }
}

Sprite Hud::glyph(char ch) const {
    int c = static_cast<unsigned char>(ch);
    if (c >= 32 && c <= 33) return assets_.obj((int)(Constants::SYMBOLS_X + (c - 32) * Constants::TILE_SIZE), (int)Constants::SYMBOLS_Y, Constants::TILE_SIZE, Constants::TILE_SIZE);
    if (c >= 40 && c <= 41) return assets_.obj((int)(Constants::NUMBERS_X + (c - 29) * Constants::TILE_SIZE), (int)Constants::NUMBERS_Y, Constants::TILE_SIZE, Constants::TILE_SIZE);
    if (c == 46) return assets_.obj((int)(Constants::NUMBERS_X - Constants::TILE_SIZE), (int)Constants::NUMBERS_Y, Constants::TILE_SIZE, Constants::TILE_SIZE);
    if (c >= 48 && c <= 58) return assets_.obj((int)(Constants::NUMBERS_X + (c - 48) * Constants::TILE_SIZE), (int)Constants::NUMBERS_Y, Constants::TILE_SIZE, Constants::TILE_SIZE);
    if (c >= 65 && c <= 90) return assets_.obj((int)(Constants::LETTERS_X + (c - 65) * Constants::TILE_SIZE), (int)Constants::LETTERS_Y, Constants::TILE_SIZE, Constants::TILE_SIZE);
    if (c >= 97 && c <= 122) return assets_.obj((int)(Constants::LETTERS_X + (c - 97) * Constants::TILE_SIZE), (int)Constants::LETTERS_Y, Constants::TILE_SIZE, Constants::TILE_SIZE);
    return Sprite{}; // space and anything else: no glyph drawn
}
