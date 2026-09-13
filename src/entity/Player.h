#pragma once

#include "../Constants.h"

// Direct port of the g_player() array from global.inc.
struct Player {
    // States (g_player(8))
    static constexpr int PLAYING = 0;
    static constexpr int FROZEN = 1;
    static constexpr int PAUSE = 2;
    static constexpr int MOVING_TO_PORTAL = 3;
    static constexpr int DEAD = 4;
    static constexpr int READY_NEXT_STAGE = 5;
    static constexpr int RESTART_STAGE = 6;
    static constexpr int CUTSCENE_STILL = 7;
    static constexpr int CUTSCENE_WALK = 8;
    static constexpr int CUTSCENE_FADE = 9; // and above

    // Power up values (g_player(5))
    static constexpr int PWR_NONE = 0;
    static constexpr int PWR_SHIELD = 1;
    static constexpr int PWR_INVISIBLE = 2;
    static constexpr int PWR_INVINCIBLE = 4;

    // Default to the spawn position so the player renders correctly even before the first
    // initStage() call (e.g. during the "STAGE 01" intro wait), instead of at (0,0).
    float x = Constants::PLAYER_INIT_COL * Constants::TILE_SIZE;
    float y = Constants::PLAYER_INIT_ROW * Constants::TILE_SIZE;
    float animCounter = 0;
    int weapon = 1;
    float speed = 0;
    int powerUp = 0;
    int shield = 0;
    int lives = 0;
    int state = READY_NEXT_STAGE;

    bool isMoving = false;
    long long lastShotTimeMs = -100000;
    bool shieldSpriteVisible = false;

    bool fireDownLastFrame = false;
    int animOffsetX = 0;

    float effectiveSpeed() const {
        return powerUp == PWR_INVINCIBLE ? Constants::PLAYER_MAX_SPEED : speed;
    }
};
