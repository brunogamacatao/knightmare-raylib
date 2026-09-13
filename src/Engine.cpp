#include "Engine.h"

#include <algorithm>

using namespace Constants;

void Engine::initStage() {
    GameState& s = state;
    s.blocks.clear();
    s.boss.reset();
    s.playerShots.clear();
    s.enemyShots.clear();
    s.obj.clear();
    s.queue.clear();
    s.portalObj = nullptr;
    s.resetGlobals();

    Player& p = s.player;
    p.x = PLAYER_INIT_COL * TILE_SIZE;
    p.y = PLAYER_INIT_ROW * TILE_SIZE;
    p.animCounter = 0;

    if (p.shield > 0) objectSystem.spawnShield();

    mapSystem.loadMap(s.stage);
    mapSystem.drawMap(s.row);

    if (s.stage == 9) {
        objectSystem.spawnPrincess();
        objectSystem.createPortal();
    }

    if (s.row > 8 || s.stage == 9) assets.playStageSong(s.stage); else assets.playBossSong(s.stage);
}

void Engine::initPlayer(int lives) {
    Player& p = state.player;
    p.animCounter = 0;
    p.weapon = 1;
    p.speed = PLAYER_INIT_SPEED;
    p.powerUp = 0;
    p.shield = 0;
    p.lives = lives;
    p.state = Player::READY_NEXT_STAGE;
}

void Engine::tick() {
    GameState& s = state;
    s.clockMs += GAME_TICK_MS;
    s.tick++;

    if (s.freezeTimer < 0 && s.row >= -1 && s.tick % 16 == 0) mapSystem.scrollMap();

    objectSystem.processInput();

    if (s.player.state == Player::MOVING_TO_PORTAL) objectSystem.autoMovePlayerToPortal();

    if (s.tick % 6 == 0) {
        s.animTick++;
        objectSystem.animatePlayer();
        if (s.boss.status > 1) bossSystem.animateBoss();
        if (s.player.state >= 20) objectSystem.fadeMap(20);
        objectSystem.animateShots();
        objectSystem.animateObjects(false);
        s.fire = false;
    }

    if (s.tick % 250 == 0) {
        if (s.stage > 1 || s.row < 125) objectSystem.enemiesFire();
    }
    if (s.boss.status > 1) bossSystem.bossFire();

    objectSystem.moveShots();
    objectSystem.moveAndProcessObjects();
    if (s.boss.status > 0) bossSystem.moveBoss();

    collisionSystem.update();
    actionQueueSystem.process();
    sweepDeadEntities();

    if (s.freezeTimer >= 0) processFreezeTimer();
    if (s.powerUpTimer >= 0) processPowerUpTimer();
}

// Compacts every unbounded pool, dropping entries marked dead earlier this tick (destroyed, or
// scrolled/flown past the visible area). Entities are only ever marked free()/isFree() during the
// tick's own update/collision passes - actually removing them is deferred to this single point so
// none of those passes need to worry about a list mutating under an in-progress iteration.
void Engine::sweepDeadEntities() {
    GameState& s = state;
    std::erase_if(s.playerShots, [](const auto& p) { return p->isFree(); });
    std::erase_if(s.enemyShots, [](const auto& p) { return p->isFree(); });
    std::erase_if(s.obj, [](const auto& p) { return p->isFree(); });
    std::erase_if(s.blocks, [](const auto& p) { return p->isFree(); });
    std::erase_if(s.queue, [](const auto& p) { return p->isFree(); });
}

bool Engine::stageFinished() const {
    const Player& p = state.player;
    return (p.state == Player::READY_NEXT_STAGE || p.state == Player::RESTART_STAGE) || p.state > 160;
}

void Engine::processFreezeTimer() {
    GameState& s = state;
    int fraction = (int)((s.freezeTimer - (int)s.freezeTimer) * 100);
    if (fraction == 0 || (s.freezeTimer < 4 && fraction == 50)) assets.playSfx("FREEZE_TICK");
    s.freezeTimer -= s.deltaTime;
}

void Engine::processPowerUpTimer() {
    GameState& s = state;
    s.powerUpTimer -= 0.025f;
    if (s.powerUpTimer < 11 && (int)(s.powerUpTimer * 100) == ((int)s.powerUpTimer) * 100) {
        assets.playSfx("POWER_UP_ENDING");
    }
    if (s.powerUpTimer < 0) {
        s.player.powerUp = s.player.shield > 0 ? Player::PWR_SHIELD : Player::PWR_NONE;
    }
}
