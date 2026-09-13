#pragma once

struct Block;

// Port of map.inc: map loading, scrolling, tile collision and destructible blocks.
class MapSystem {
public:
    explicit MapSystem(class Engine& engine) : engine_(engine) {}

    void loadMap(int stage);
    bool isSolid(int row, int col) const;

    // Returns true if the player's 16x16 box at buffer-space (px,py) overlaps any solid tile.
    bool mapCollide(float px, float py) const;

    void checkScrollCollision();
    void calculateStartRow();

    // Initial screen fill. Spawns pre-placed blocks only (enemies never spawn for rows visible at
    // stage start - only rows newly scrolled in ever get enemies, matching the original).
    void drawMap(int row);

    void scrollMap();

    int blockMaxHits(const Block& b) const;
    void spawnBlock(int row, int col, int type, float offsetY);
    void collectBlockBonus(Block& b);
    void replaceBlock(Block* b);

    void makeBridgeGround(int row, int col, bool down);
    void makeBlockSolid(int row, int col);
    void makeTileSolid(int row, int col, bool solid);

private:
    Engine& engine_;

    void checkBossTriggers(int classicRow);
    void spawnRowContent(int row, bool spawn);
    void setBlockRegion(Block& b, int offset);
};
