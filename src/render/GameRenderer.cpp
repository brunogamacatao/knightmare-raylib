#include "GameRenderer.h"

#include <algorithm>
#include <cmath>
#include "Draw.h"
#include "../Engine.h"

using namespace Constants;

GameRenderer::GameRenderer(Engine& engine, const Assets& assets) : engine_(engine), assets_(assets), hud_(assets) {}

void GameRenderer::resize(int width, int height) {
    screenW_ = width;
    screenH_ = height;

    hudHeightPx = height * 0.075f;
    // The world's viewport is NOT shortened to leave room for the on-screen touch controls bar:
    // that bar (see VirtualControls::render(), drawn after this) is a translucent overlay on top
    // of the game, not reserved empty space - map content keeps scrolling underneath and dims/
    // reappears through it exactly like it does through nothing everywhere else, rather than being
    // hard-cut at a boundary the player would otherwise never see coming. Only the opaque HUD strip
    // at the top actually excludes the world.
    float availW = width * 0.96f;
    float availH = std::max(20.0f, height - hudHeightPx);

    // Fill both width and height exactly: width sets the scale (so columns always match the
    // screen edge-to-edge), and the world's rendered height simply grows to fill whatever vertical
    // space is left over, instead of preserving the original 256x176 aspect ratio.
    float scale = availW / WORLD_W;
    worldH_ = availH / scale;

    viewportX_ = (int)((width - availW) / 2.0f);
    viewportY_ = (int)hudHeightPx;
    viewportW_ = (int)availW;
    viewportH_ = (int)availH;

    worldCamera_.target = {0, 0};
    worldCamera_.offset = {(float)viewportX_, (float)viewportY_};
    worldCamera_.rotation = 0;
    worldCamera_.zoom = scale;

    engine_.state.extraRows = (int)std::max(0.0f, std::ceil((worldH_ - CLASSIC_H) / TILE_SIZE));
}

void GameRenderer::render() {
    GameState& s = engine_.state;

    BeginScissorMode(viewportX_, viewportY_, viewportW_, viewportH_);
    BeginMode2D(worldCamera_);
    drawMap(s);
    drawBlocks(s);
    drawObjectsLayer(s, 4);
    drawObjectsLayer(s, 3);
    drawObjectsLayer(s, 2);
    bool playerBehind = s.player.state == Player::MOVING_TO_PORTAL;
    if (playerBehind) drawPlayer(s);
    drawObjectsLayer(s, 1);
    drawShots(s);
    if (!playerBehind) drawPlayer(s);
    drawOverlay(s);
    drawCenterMessage(s);
    EndMode2D();
    EndScissorMode();

    drawHud(s);
}

void GameRenderer::drawMap(const GameState& s) {
    if (!s.map) return;
    // The classic 22-row (176px) window is anchored to the TOP of the screen, exactly like the
    // original, non-extended game - one extra row above is kept so a partially-scrolled row never
    // exposes a gap at the top edge. On a taller portrait screen the remaining space below is
    // filled with extra trailing rows (already-passed terrain - purely a backdrop, see
    // MapSystem's revealedExtraRows), growing as the map scrolls instead of appearing instantly.
    int topR = -1;
    int bottomR = SCREEN_ROWS + 1 + s.revealedExtraRows;
    for (int r = topR; r <= bottomR; r++) {
        int mapRow = s.row + r;
        if (mapRow < 0 || mapRow >= MAP_ROWS) continue;
        float y = r * (float)TILE_SIZE + s.tilePx;
        for (int col = 0; col < MAP_COLS; col++) {
            int tileId = s.map->tileId(mapRow, col);
            int tx = (tileId % TILES_COLS) * TILE_SIZE;
            int ty = (tileId / TILES_COLS) * TILE_SIZE + TILES_OFFSET[s.stage - 1];
            Sprite region = assets_.mapRegion(tx, ty, TILE_SIZE, TILE_SIZE);
            RDraw::sprite(region, col * (float)TILE_SIZE, y, TILE_SIZE, TILE_SIZE);
        }
    }
}

void GameRenderer::drawBlocks(const GameState& s) {
    for (auto& bPtr : s.blocks) {
        const Block& b = *bPtr;
        if (b.isFree() || b.hidden || !b.region.valid()) continue;
        RDraw::sprite(b.region, b.x, spriteScreenY(b.y), TILE_SIZE_X2, TILE_SIZE_X2);
    }
}

void GameRenderer::drawObjectsLayer(const GameState& s, int layer) {
    for (auto& oPtr : s.obj) {
        const GameObj& o = *oPtr;
        if (o.id == 0 || o.hidden || !o.region.valid() || o.layer != layer) continue;
        RDraw::sprite(o.region, o.x + o.renderOffsetX, spriteScreenY(o.y + o.renderOffsetY), o.region.w, o.region.h, o.flipX);
    }
}

void GameRenderer::drawShots(const GameState& s) {
    auto drawList = [this](const auto& shots) {
        for (auto& shPtr : shots) {
            const Shot& sh = *shPtr;
            if (sh.isFree() || !sh.region.valid()) continue;
            float w = (float)sh.region.w, h = (float)sh.region.h;
            float screenY = spriteScreenY(sh.y);
            RDraw::spriteRotatedCentered(sh.region, sh.x + w / 2.0f, screenY + h / 2.0f, w, h, sh.rot * 90.0f);
        }
    };
    drawList(s.playerShots);
    drawList(s.enemyShots);
}

void GameRenderer::drawPlayer(const GameState& s) {
    const Player& p = s.player;
    Sprite region = assets_.playerRegion(p);
    if (region.valid()) {
        RDraw::sprite(region, p.x, spriteScreenY(p.y), region.w, region.h, false);
    }
    if (p.powerUp == Player::PWR_SHIELD && p.shieldSpriteVisible) {
        Sprite shield = assets_.shieldRegion(p.shield);
        if (shield.valid()) {
            RDraw::sprite(shield, p.x, spriteScreenY(p.y - TILE_SIZE), shield.w, shield.h);
        }
    }
}

void GameRenderer::drawOverlay(const GameState& s) {
    if (s.blinkActive) {
        Color c = s.blinkGray ? Color{128, 128, 128, 255} : BLACK;
        DrawRectangle(0, 0, (int)WORLD_W, (int)worldH_, c);
    }
    if (s.fadeActive) {
        // Paints black everywhere in the world rect except the shrinking [left,top,w,h] window -
        // this is the "fade to a point" stage-intro transition. The bottom edge only masks down to
        // the classic playfield height, leaving any cosmetic extra trailing rows on a tall portrait
        // screen unmasked below it, matching the original (non-extended) effect exactly.
        float left = s.fadeX, top = s.fadeY - TILE_SIZE_X2, w = s.fadeW, h = s.fadeH;
        if (top > 0) DrawRectangle(0, 0, (int)WORLD_W, (int)top, BLACK);
        float bottomY = top + h;
        if (bottomY < CLASSIC_H) DrawRectangle(0, (int)bottomY, (int)WORLD_W, (int)(CLASSIC_H - bottomY), BLACK);
        if (left > 0) DrawRectangle(0, (int)top, (int)left, (int)h, BLACK);
        float rightX = left + w;
        if (rightX < WORLD_W) DrawRectangle((int)rightX, (int)top, (int)(WORLD_W - rightX), (int)h, BLACK);
    }
}

void GameRenderer::drawCenterMessage(const GameState& s) {
    if (s.centerMessage.empty()) return;
    float tile = TILE_SIZE;
    float w = hud_.width(s.centerMessage, tile);
    hud_.draw(s.centerMessage, (WORLD_W - w) / 2.0f, worldH_ / 2.0f, tile);
}

void GameRenderer::drawHud(const GameState& s) {
    float baseTile = hudHeightPx * 0.42f;
    float pad = baseTile * 0.4f;
    float rowY1 = pad;
    float rowY2 = rowY1 + baseTile + pad * 0.3f;
    float availW = screenW_ - pad * 2;

    std::string stageTxt = "STAGE " + std::string(s.stage < 10 ? "0" : "") + std::to_string(s.stage);
    drawHudRow("SCORE " + std::to_string(s.score), "HI " + std::to_string(s.hiscore), pad, rowY1, baseTile, availW);
    drawHudRow("LIVES " + std::to_string(std::max(0, s.player.lives)), stageTxt, pad, rowY2, baseTile, availW);

    float rowY3 = rowY2 + baseTile + pad * 0.3f;
    if (s.freezeTimer >= 0) {
        int lsh = (int)s.freezeTimer;
        int rsh = (int)((s.freezeTimer - lsh) * 60);
        std::string txt = "FREEZE " + (lsh < 10 ? std::string("0") : std::string("")) + std::to_string(lsh) + ":" +
                           (rsh < 10 ? std::string("0") : std::string("")) + std::to_string(rsh);
        float t = fitTile(txt, baseTile, availW);
        hud_.draw(txt, pad, rowY3, t);
    }
    if (s.powerUpTimer >= 0) {
        int v = (int)s.powerUpTimer;
        std::string txt = "POWER " + (v < 10 ? std::string("0") : std::string("")) + std::to_string(v);
        float t = fitTile(txt, baseTile, availW);
        hud_.draw(txt, screenW_ - pad - hud_.width(txt, t), rowY3, t);
    }
}

// Draws a left string flush-left and a right string flush-right, shrinking the glyph size just
// enough for both (plus a gap) to fit within availW - score/hiscore can grow to many digits, so a
// fixed size would eventually overflow past the edge of the screen.
void GameRenderer::drawHudRow(const std::string& left, const std::string& right, float x, float y, float baseTile, float availW) {
    float gapChars = 2.0f;
    float totalChars = (float)(left.size() + right.size()) + gapChars;
    float tile = std::min(baseTile, availW / totalChars);
    hud_.draw(left, x, y, tile);
    hud_.draw(right, x + availW - hud_.width(right, tile), y, tile);
}

float GameRenderer::fitTile(const std::string& text, float baseTile, float availW) const {
    float w = hud_.width(text, baseTile);
    return w > availW ? baseTile * (availW / w) : baseTile;
}
