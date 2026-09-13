#pragma once

#include <array>
#include <cstdint>

// Direct port of constants.inc / global.inc numeric tables from the original MMBasic source.
namespace Constants {

inline constexpr int TILE_SIZE = 8;
inline constexpr int TILE_SIZE_X2 = TILE_SIZE * 2;
inline constexpr int TILE_SIZE_X4 = TILE_SIZE * 4;
inline constexpr int TILES_COLS = 32;

inline constexpr int MAP_COLS = 32;
inline constexpr int MAP_ROWS = 238;
inline constexpr int MAP_COLS_0 = MAP_COLS - 1;
inline constexpr int MAP_ROWS_0 = MAP_ROWS - 1;

inline constexpr int SCREEN_ROWS = 22;
inline constexpr int SCREEN_COLS = MAP_COLS;
inline constexpr int SCREEN_WIDTH = MAP_COLS * TILE_SIZE;    // 256
inline constexpr int SCREEN_HEIGHT = SCREEN_ROWS * TILE_SIZE; // 176
inline constexpr float SCREEN_CENTER_W = SCREEN_WIDTH / 2.0f;
inline constexpr float SCREEN_CENTER_H = SCREEN_HEIGHT / 2.0f;
inline constexpr float EDGE_LEFT = TILE_SIZE_X4;
inline constexpr float EDGE_RIGHT = SCREEN_WIDTH - TILE_SIZE_X4;

// Fraction of the screen height reserved at the bottom for the on-screen controls bar, shared by
// GameRenderer (which leaves this much room above it) and VirtualControls (which draws and
// hit-tests the bar itself), so the two always agree on where it sits.
inline constexpr float VIRTUAL_CONTROLS_HEIGHT_FRACTION = 0.24f;

inline constexpr int GAME_TICK_MS = 1000 / 120;

// Minimum time between shots, per player weapon id (1..11). Firing used to also be gated by
// whether the previous shot(s) still occupied a slot in the fixed-size shot pool; now that the
// pool is unbounded (see GameState), pacing is entirely down to this interval. Most weapons keep
// the original single global cooldown (150ms); the triple-flame breath (3/8) gets a longer one so
// it doesn't fire a fresh volley of 3 before the last one has mostly cleared the screen. The
// boomerang (4/9) keeps the baseline 150ms too - its own pacing comes from a separate in-flight
// count cap in ObjectSystem::fire(), not this cooldown (up to 2-3 can be out at once, and throwing
// another is only blocked while that many still are, matching the original "wait for a slot"
// behaviour rather than a fixed timer).
inline constexpr std::array<int, 12> WEAPON_COOLDOWN_MS = {150, 150, 150, 260, 150, 150, 150, 150, 260, 150, 150, 150};

inline int weaponCooldownMs(int weapon) {
    return (weapon >= 0 && weapon < static_cast<int>(WEAPON_COOLDOWN_MS.size())) ? WEAPON_COOLDOWN_MS[weapon] : 150;
}

inline constexpr int LEFT = -1;
inline constexpr int RIGHT = 1;

inline constexpr float PLAYER_INIT_SPEED = 60;
inline constexpr int PLAYER_INIT_COL = SCREEN_COLS / 2 - 1;
inline constexpr int PLAYER_INIT_ROW = SCREEN_ROWS - 1;
inline constexpr float PLAYER_SPEED_INC = 20;
inline constexpr float PLAYER_MAX_SPEED = PLAYER_INIT_SPEED + PLAYER_SPEED_INC * 3;
inline constexpr int NEW_LIFE_POINTS = 100000;
inline constexpr int SHIELD_MAX_HITS = 30;

inline constexpr float BOSS_SPEED = 60;
inline constexpr float MAX_ENEMIES_SHOOT_Y = SCREEN_HEIGHT * 0.6f;

inline constexpr int BLOCK_HITS = 5;
inline constexpr int BLOCK_POINTS = 500;
inline constexpr float FREEZE_TIME = 10;
inline constexpr float POWER_UP_TIME = 45;

inline constexpr int KNIGHT_CHANGE_DIRECT_DIST_PX = 55;
inline constexpr int KNIGHT_MAX_HORIZONTAL_DIST_PX = 48;
inline constexpr float CLOUD_ATTACK_SPEED = 250;
inline constexpr int DEMON_CHANGE_DIRECT_DIST_PX = 55;
inline constexpr int DEMON_ATTACK_DIST_PX = 60;

// Tileset offsets (pixels) added to the map tile Y address, per stage (index = stage-1).
inline constexpr std::array<int, 9> TILES_OFFSET = {
    0, TILE_SIZE * 3, TILE_SIZE * 5, TILE_SIZE * 9, TILE_SIZE * 12,
    TILE_SIZE * 15, TILE_SIZE * 15, TILE_SIZE * 19, TILE_SIZE * 19
};

// Boss life per stage. Index 0 unused, 1..8 stages, index 8 also reused for stage-8 boss eye life calc.
inline constexpr std::array<int, 9> BOSS_LIFE = {0, 20, 48, 30, 48, 40, 40, 40, 24};
// Difficulty multiplier: Easy, Normal, Hard
inline constexpr std::array<float, 3> BOSS_DIFFICULTY = {0.5f, 1.0f, 1.5f};

// Chance of enemy shoot per stage (rows = difficulty 0..2, cols = stage 0..8)
inline constexpr std::array<std::array<float, 9>, 3> SHOOT_CHANCE = {{
    {0, 0.3f, 0.3f, 0.4f, 0.4f, 0.5f, 0.5f, 0.6f, 0.6f},
    {0, 0.5f, 0.5f, 0.6f, 0.6f, 0.7f, 0.7f, 0.8f, 0.8f},
    {0, 0.7f, 0.7f, 0.8f, 0.8f, 0.9f, 0.9f, 1.0f, 1.0f},
}};

inline constexpr std::array<int, 26> PLAYER_DEATH_ANIM = {0,12,0,12,0,12,0,12,0,12,0,12,0,12,0,12,14,16,18,20,22,22,22,22,22,22};
inline constexpr int PLAYER_SKIN_Y = 0;
inline constexpr int PLAYER_SKIN1_X_L = 0;
inline constexpr int PLAYER_SKIN1_X_R = TILE_SIZE * 2;

// Points awarded per enemy/object id (index = obj id)
inline constexpr std::array<int, 39> OBJ_POINTS = {0,10,50,50,100,200,50,100,200,200,100,100,300,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

// Weapon crystal animation frames: row = (gpr2 clamped), col = frame index (0..3)
inline constexpr std::array<std::array<int, 6>, 4> WEAPON_ANIM = {{
    {0,3,5,7,9,11},
    {1,4,6,8,10,12},
    {0,3,5,7,9,11},
    {2,1,1,1,1,1}
}};
inline constexpr std::array<std::array<int, 5>, 4> PUP_ANIM = {{
    {0,3,6,9,12},
    {1,4,7,10,13},
    {0,3,6,9,12},
    {2,5,8,11,14}
}};
inline constexpr std::array<int, 16> FIRE_ANIM = {1,0,1,0,1,0,2,1,2,1,2,1,3,2,3,2};

inline constexpr float SYMBOLS_X = 0, SYMBOLS_Y = 48;
inline constexpr float NUMBERS_X = 24, NUMBERS_Y = 48;
inline constexpr float LETTERS_X = 0, LETTERS_Y = 56;
inline constexpr float TRANSPARENT_BLOCK_X = 96, TRANSPARENT_BLOCK_Y = 144;

// Object tile sheet metadata: tileX, tileY, tileW, tileH, dataId (index into OBJ_DATA).
struct ObjTile {
    int x = 0, y = 0, w = 0, h = 0, dataId = 0;
};

// Object base movement/behaviour data: speedX, speedY, gpr1, gpr2, gpr3.
struct ObjData {
    float speedX = 0, speedY = 0, gpr1 = 0, gpr2 = 0, gpr3 = 0;
    float operator[](int i) const {
        switch (i) {
            case 0: return speedX;
            case 1: return speedY;
            case 2: return gpr1;
            case 3: return gpr2;
            default: return gpr3;
        }
    }
};

inline constexpr std::array<ObjTile, 61> buildObjTiles() {
    std::array<ObjTile, 61> t{};
    t[1] = {1,17,14,14,1};      // Blob
    t[2] = {32,16,16,11,2};     // Bat
    t[3] = {32,16,16,11,3};     // Bat wave
    t[4] = {64,16,16,16,4};     // Knight
    t[5] = {113,17,14,14,5};    // Cloud
    t[6] = {144,16,16,16,6};    // Blue demon
    t[7] = {176,16,16,16,7};    // Skeleton
    t[8] = {240,17,16,15,8};    // Demon
    t[9] = {16,136,15,16,9};    // Death ghost
    t[10] = {64,136,16,16,10};  // Zombie
    t[11] = {16,120,16,16,11};  // Ghost
    t[12] = {209,2,13,13,12};   // Yellow thing
    t[13] = {224,1,15,14,13};   // Red thing
    t[14] = {32,16,16,11,14};   // Bat reverse
    t[15] = {0,120,16,16,15};   // Sorcerer
    t[16] = {32,136,15,16,16};  // Red Death ghost

    t[20] = {0,64,16,16,20};    // Weapon crystal
    t[21] = {0,80,16,16,21};    // Power up crystal
    t[22] = {0,104,16,16,0};    // Blocks

    t[23] = {22,38,4,4,0};      // Dot bullet
    t[24] = {35,35,10,10,0};    // Energy ray
    t[25] = {50,37,12,6,0};     // Bone
    t[26] = {66,35,11,11,0};
    t[27] = {85,34,6,12,0};
    t[28] = {99,35,9,9,0};      // White explosion
    t[29] = {133,34,5,14,0};    // Arrow
    t[30] = {145,38,13,5,0};
    t[31] = {163,35,11,11,0};
    t[32] = {177,34,13,13,0};   // Axe
    t[33] = {48,120,16,16,0};   // Scythe
    t[34] = {112,104,16,16,0};  // Fire
    t[35] = {112,120,16,16,0};  // Big fire
    t[36] = {136,48,16,7,0};    // Shield

    t[37] = {200,98,40,6,0};    // Boss shadow
    t[38] = {184,49,24,6,0};    // Big shadow
    t[39] = {0,33,16,6,0};      // Shadow

    t[40] = {209,50,5,13,0};    // Arrow (player)
    t[41] = {210,66,12,13,0};   // Twin arrows
    t[42] = {218,54,4,10,0};    // Triple flames
    t[43] = {209,35,5,10,0};    // Boomerang
    t[44] = {218,37,10,5,0};
    t[45] = {225,48,6,16,0};    // Sword
    t[46] = {226,64,13,16,0};   // Double sword
    t[47] = {232,49,8,14,0};    // Fire arrow
    t[48] = {184,136,32,8,0};   // Bridge
    t[49] = {240,192,16,8,0};   // Terrain
    t[50] = {176,128,16,16,0};  // Boss eyes
    t[51] = {0,136,16,24,0};    // Princess
    return t;
}

inline constexpr std::array<ObjData, 22> buildObjData() {
    std::array<ObjData, 22> d{};
    d[1] = {0,0,20,1,0};                         // Blob
    d[2] = {75,65,1,90,0};                       // Bat
    d[14] = {50,-35,1,90,0};                     // Bat reverse
    d[3] = {120,33,1,0,0};                       // Bat wave
    d[4] = {40,40,3,0,0};                        // Knight
    d[5] = {25,360,0,0,0};                       // Cloud
    d[6] = {70,70,1,0,0};                        // Blue demon
    d[7] = {50,15,3,0,0};                        // Skeleton
    d[8] = {160,160,1,0,0};                      // Black demon
    d[9] = {90,80,1,0,0};                        // Death ghost
    d[10] = {0,25,1,60,0};                       // Zombie
    d[11] = {90,31,1,0,0};                       // Ghost
    d[12] = {240,240,1,0,0};                     // Yellow thing
    d[13] = {140,50,1,0,0};                      // Red thing
    d[15] = {0,20,1,0,0};                        // Sorcerer
    d[16] = {40,20,1,0,0};                       // Red death ghost
    d[20] = {50,20,0,0,-1};                      // Weapon crystal
    d[21] = {50,20,0,0,-1};                      // Power-up crystal
    return d;
}

inline constexpr std::array<ObjTile, 61> OBJ_TILE = buildObjTiles();
inline constexpr std::array<ObjData, 22> OBJ_DATA = buildObjData();

} // namespace Constants
