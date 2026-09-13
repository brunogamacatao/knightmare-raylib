#include "MapSystem.h"

#include <algorithm>
#include <cmath>
#include "../Engine.h"

using namespace Constants;

void MapSystem::loadMap(int stage) {
    engine_.state.map = std::make_unique<MapData>(engine_.assets.stages[stage]->copy());
}

bool MapSystem::isSolid(int row, int col) const {
    const GameState& s = engine_.state;
    if (row < 0 || row >= MAP_ROWS) return false;
    int c = ((col % MAP_COLS) + MAP_COLS) % MAP_COLS;
    return s.map->isSolid(row, c);
}

bool MapSystem::mapCollide(float px, float py) const {
    const GameState& s = engine_.state;
    // py is in the sprite/object "buffer space" (see GameRenderer::spriteScreenY); convert to the
    // same classic map-space the tile grid itself is addressed in before turning it into a row.
    float classicYTop = py - TILE_SIZE_X2;
    float classicYBottom = classicYTop + TILE_SIZE_X2 - 1;
    int rowTop = s.row + (int)std::floor((classicYTop - s.tilePx) / (float)TILE_SIZE);
    int rowBottom = s.row + (int)std::floor((classicYBottom - s.tilePx) / (float)TILE_SIZE);

    int colLeft = (int)std::floor(px / (float)TILE_SIZE);
    int colRight = (int)std::floor((px + TILE_SIZE_X2 - 1) / (float)TILE_SIZE);

    for (int row = rowTop; row <= rowBottom; row++) {
        for (int col = colLeft; col <= colRight; col++) {
            if (isSolid(row, col)) return true;
        }
    }
    return false;
}

void MapSystem::checkScrollCollision() {
    GameState& s = engine_.state;
    if (mapCollide(s.player.x, s.player.y)) {
        s.player.y += 2;
        if (std::floor(s.player.y + TILE_SIZE / 2.0f) > s.visibleBottomY()) {
            engine_.objectSystem.killPlayer();
        }
    }
}

void MapSystem::calculateStartRow() {
    GameState& s = engine_.state;
    if (s.row <= 10) { s.row = 0; return; }
    bool solid = true;
    s.row += 5;
    while (solid) {
        solid = isSolid(s.row + PLAYER_INIT_ROW, PLAYER_INIT_COL);
        solid = solid && isSolid(s.row + PLAYER_INIT_ROW, PLAYER_INIT_COL + 1);
        solid = solid && isSolid(s.row + PLAYER_INIT_ROW + 1, PLAYER_INIT_COL);
        solid = solid && isSolid(s.row + PLAYER_INIT_ROW + 1, PLAYER_INIT_COL);
        if (solid) s.row++;
    }
}

void MapSystem::drawMap(int row) {
    GameState& s = engine_.state;
    row = std::min(MAP_ROWS_0 - SCREEN_ROWS, row);
    s.row = row;
    s.tilePx = 0;
    for (int r = SCREEN_ROWS + row - 1 + s.revealedExtraRows; r >= row - 2; r--) {
        spawnRowContent(r, false);
    }
}

void MapSystem::scrollMap() {
    GameState& s = engine_.state;
    if (!s.scrollOn) return;

    // Scroll block sprites down with the map.
    for (auto& bPtr : s.blocks) {
        Block& b = *bPtr;
        if (b.isFree()) continue;
        b.y += 1;
        if (b.y > s.visibleBottomY() + TILE_SIZE_X2) engine_.objectSystem.destroyBlock(b);
    }

    if (s.boss.status > 0) engine_.bossSystem.scrollBoss();

    checkScrollCollision();
    s.tilePx++;

    if (s.tilePx == TILE_SIZE) {
        s.tilePx = 0;
        s.row--;
        // Boss/song triggers fire when the *classic* viewport reaches these dungeon depths.
        checkBossTriggers(s.row);
        // Enemies/objects/blocks always spawn exactly at the classic top row, exactly like the
        // original 22-row game - there is no extra look-ahead above it, so the player can never
        // out-climb where enemies actually appear.
        spawnRowContent(s.row, true);

        // The extra trailing rows a tall portrait screen shows *below* the classic window are
        // purely cosmetic (already-passed terrain the renderer keeps drawing); this just grows how
        // many of them are considered "revealed" by one row per scroll step, until the whole screen
        // is filled.
        if (s.revealedExtraRows < s.extraRows) s.revealedExtraRows++;
    }
}

void MapSystem::checkBossTriggers(int classicRow) {
    GameState& s = engine_.state;
    if (classicRow < -1) {
        if (s.stage != 9) engine_.bossSystem.activateBoss();
        return;
    }
    if (classicRow < 0) return;
    if (classicRow == 7) engine_.bossSystem.spawnBoss();
    if (classicRow == 8) {
        engine_.assets.playBossSong(s.stage);
        engine_.objectSystem.destroyEnemiesShots();
        engine_.objectSystem.killAllEnemies(false);
    }
}

void MapSystem::spawnRowContent(int row, bool spawn) {
    GameState& s = engine_.state;
    if (row < 0 || row >= MAP_ROWS) return;
    float topOffset = (row - s.row) * (float)TILE_SIZE;

    for (int col = MAP_COLS_0; col >= 0; col--) {
        int objId = s.map->objectId(row, col);
        if (!spawn && objId != 22) continue;
        if (objId == 0) continue;
        int extra = s.map->objectExtra(row, col);
        switch (objId) {
            case 20:
            case 21: {
                float x = std::min<float>(SCREEN_WIDTH - TILE_SIZE * 6, std::max<float>(TILE_SIZE_X4, s.player.x));
                engine_.objectSystem.spawnObject(objId, x, topOffset, 0);
                break;
            }
            case 22:
                spawnBlock(row, col, extra, topOffset + TILE_SIZE_X2);
                break;
            default: {
                int cfg = (s.difficulty == 0) ? std::min(3, extra) : extra;
                engine_.objectSystem.spawnObject(objId, col * TILE_SIZE, topOffset, cfg);
                break;
            }
        }
    }
}

int MapSystem::blockMaxHits(const Block& b) const {
    if (b.type == Block::COLLECTED) return 0;
    return (b.type > 1 && b.type < 5) ? BLOCK_HITS * 2 : BLOCK_HITS;
}

void MapSystem::spawnBlock(int row, int col, int type, float offsetY) {
    auto b = std::make_unique<Block>();
    b->type = type;
    b->hits = 0;
    b->row = row;
    b->col = col;
    b->x = col * TILE_SIZE;
    b->y = offsetY;
    b->hidden = false;
    engine_.state.blocks.push_back(std::move(b));
}

void MapSystem::collectBlockBonus(Block& b) {
    GameState& s = engine_.state;
    if (b.hits < blockMaxHits(b) || b.type > 4) return;
    switch (b.type) {
        case Block::ROOK:
            engine_.powerUpSystem.incrementScore(BLOCK_POINTS);
            engine_.assets.playSfx("POINTS");
            break;
        case Block::KNIGHT:
            engine_.objectSystem.killAllEnemies(true);
            engine_.objectSystem.destroyEnemiesShots();
            break;
        case Block::QUEEN:
            engine_.powerUpSystem.updateLife(1);
            break;
        case Block::KING:
            s.freezeTimer = FREEZE_TIME;
            engine_.objectSystem.destroyEnemiesShots();
            break;
    }
    b.type = Block::COLLECTED;
    engine_.actionQueueSystem.enqueueBlock(1, 22, &b, 1);
}

void MapSystem::replaceBlock(Block* bp) {
    if (!bp) return;
    Block& b = *bp;
    int maxHits = blockMaxHits(b);
    if (b.type != Block::COLLECTED && b.hits > 1 && b.hits < maxHits) return;

    int offset = (b.type == Block::COLLECTED || b.hits >= maxHits) ? TILE_SIZE_X2 * b.type : 0;

    if (b.type == Block::COLLECTED) {
        // The original bakes this into the scrolling tile buffer and frees the sprite; we have no
        // such buffer, so just keep the sprite showing the resolved look permanently instead.
        setBlockRegion(b, offset);
    } else if (b.type == Block::BRIDGE && b.hits >= maxHits) {
        engine_.objectSystem.spawnBridge(b.x, b.y, false);
        makeBridgeGround(b.row + 1, b.col - 1, false);
        engine_.objectSystem.destroyBlock(b);
        engine_.assets.playSfx("BLOCK_OPEN");
    } else {
        setBlockRegion(b, offset);
        if (b.type == Block::BARRIER && b.hits == maxHits) {
            makeBlockSolid(b.row, b.col);
        }
    }
}

void MapSystem::setBlockRegion(Block& b, int offset) {
    const ObjTile& t = OBJ_TILE[22];
    b.region = engine_.assets.obj(t.x + offset, t.y, t.w, t.h);
}

void MapSystem::makeBridgeGround(int row, int col, bool down) {
    for (int i = 0; i < 4; i++) {
        makeTileSolid(row, col, false);
        makeTileSolid(row, col + 1, false);
        makeTileSolid(row, col + 2, false);
        makeTileSolid(row, col + 3, false);
        row += down ? 1 : -1;
    }
}

void MapSystem::makeBlockSolid(int row, int col) {
    makeTileSolid(row, col, true);
    makeTileSolid(row, col + 1, true);
    makeTileSolid(row + 1, col, true);
    makeTileSolid(row + 1, col + 1, true);
}

void MapSystem::makeTileSolid(int row, int col, bool solid) {
    if (row < 0 || row >= MAP_ROWS || col < 0 || col >= MAP_COLS) return;
    engine_.state.map->setSolid(row, col, solid);
}
