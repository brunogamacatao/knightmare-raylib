#pragma once

#include "Assets.h"
#include "GameState.h"
#include "input/GameInput.h"
#include "systems/ActionQueueSystem.h"
#include "systems/BossSystem.h"
#include "systems/CollisionSystem.h"
#include "systems/MapSystem.h"
#include "systems/ObjectSystem.h"
#include "systems/PowerUpSystem.h"

// Wires all systems together and drives the fixed-timestep simulation loop, mirroring the
// run_stage() do-loop body in km.bas (each call to tick() == one GAME_TICK_MS iteration).
class Engine {
public:
    // Non-const: playing sfx/music (see Assets::playSfx etc.) mutates Assets' internal music-stream
    // state, and every system reaches sound playback through this reference.
    Assets& assets;
    GameState state;
    GameInput* input = nullptr;

    MapSystem mapSystem;
    ObjectSystem objectSystem;
    BossSystem bossSystem;
    CollisionSystem collisionSystem;
    PowerUpSystem powerUpSystem;
    ActionQueueSystem actionQueueSystem;

    explicit Engine(Assets& assets)
        : assets(assets), mapSystem(*this), objectSystem(*this), bossSystem(*this),
          collisionSystem(*this), powerUpSystem(*this), actionQueueSystem(*this) {}

    void initStage();
    void initPlayer(int lives);

    // One fixed-timestep simulation tick (GAME_TICK_MS).
    void tick();

    bool stageFinished() const;

private:
    void sweepDeadEntities();
    void processFreezeTimer();
    void processPowerUpTimer();
};
