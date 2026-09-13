#pragma once

#include "raylib.h"
#include "Hud.h"
#include "../Constants.h"

class Engine;
struct GameState;

// Draws the game. Redesigned for a portrait phone screen: a HUD bar on top, the original 256x176
// playfield scaled and centered below it, and the virtual controls area reserved at the bottom
// (rendered separately by VirtualControls).
//
// Coordinate note: every gameplay position (player/object/shot/block x,y) is expressed in the
// original engine's top-down "buffer space" (y grows downward, with a 16px pad at the top - see
// spriteScreenY()), which is exactly raylib's native screen convention. Unlike the original
// libGDX port (whose OpenGL-style y-up camera needed a flip() on every single draw call), this
// renderer draws world positions directly with no flip.
class GameRenderer {
public:
    GameRenderer(Engine& engine, const Assets& assets);

    void resize(int width, int height);
    void render();

    float hudHeightPx = 0;

private:
    static constexpr float WORLD_W = Constants::SCREEN_WIDTH;
    // Fixed height of the original playfield (22 rows). Gameplay (player/enemies/blocks/HUD text
    // inside the world) stays anchored to this, top-aligned within the taller world, regardless of
    // screen shape - exactly like the original, non-extended game.
    static constexpr float CLASSIC_H = Constants::SCREEN_HEIGHT;

    Engine& engine_;
    const Assets& assets_;
    Hud hud_;

    Camera2D worldCamera_{};
    int screenW_ = 0, screenH_ = 0;
    int viewportX_ = 0, viewportY_ = 0, viewportW_ = 0, viewportH_ = 0;
    // Actual rendered world height in world units: grows past CLASSIC_H to fill a tall portrait
    // screen edge-to-edge. The classic playfield stays pinned to the top of this space; the extra
    // room below is filled with cosmetic trailing map rows instead of leaving black letterbox bars
    // (see MapSystem's revealedExtraRows).
    float worldH_ = CLASSIC_H;

    static float spriteScreenY(float logicalTopY) { return logicalTopY - Constants::TILE_SIZE_X2; }

    void drawMap(const GameState& s);
    void drawBlocks(const GameState& s);
    void drawObjectsLayer(const GameState& s, int layer);
    void drawShots(const GameState& s);
    void drawPlayer(const GameState& s);
    void drawOverlay(const GameState& s);
    void drawCenterMessage(const GameState& s);
    void drawHud(const GameState& s);
    void drawHudRow(const std::string& left, const std::string& right, float x, float y, float baseTile, float availW);
    float fitTile(const std::string& text, float baseTile, float availW) const;
};
