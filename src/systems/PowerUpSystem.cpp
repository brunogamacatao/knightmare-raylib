#include "PowerUpSystem.h"

#include "../Engine.h"

using namespace Constants;

void PowerUpSystem::changeWeapon(GameObj& o) {
    if (o.id != 20) return;
    int variant = (int)o.gpr2;
    if (variant <= 2) {
        incrementScore(1000);
        engine_.assets.playSfx("POINTS");
    } else {
        incrementScore(200);
        engine_.assets.playSfx("CHRYSTAL");
        int weapon = variant - 1;
        if (weapon == 5) increasePlayerSpeed();
        Player& p = engine_.state.player;
        p.weapon = weapon + ((p.weapon == weapon || p.weapon == weapon + 5) ? 5 : 0);
    }
}

void PowerUpSystem::powerUp(GameObj& o) {
    if (o.id != 21) return;
    GameState& s = engine_.state;
    int variant = (int)o.gpr2;
    if (variant <= 2) {
        incrementScore(1000);
        engine_.assets.playSfx("POINTS");
        return;
    }
    switch (variant) {
        case 3: increasePlayerSpeed(); break;
        case 4: engine_.actionQueueSystem.enqueue(1, 36, 0, 0, 0, 0); break;
        case 5: s.player.powerUp = Player::PWR_INVISIBLE; s.powerUpTimer = POWER_UP_TIME; break;
        case 6: s.player.powerUp = Player::PWR_INVINCIBLE; s.powerUpTimer = POWER_UP_TIME; break;
    }
    incrementScore(200);
    engine_.assets.playSfx("CHRYSTAL");
}

void PowerUpSystem::increasePlayerSpeed() {
    Player& p = engine_.state.player;
    if (p.speed < PLAYER_MAX_SPEED) p.speed += PLAYER_SPEED_INC;
}

void PowerUpSystem::incrementScore(int points) {
    GameState& s = engine_.state;
    int before = s.score / NEW_LIFE_POINTS;
    s.score += points;
    if (s.score / NEW_LIFE_POINTS > before) updateLife(1);
    if (s.score > s.hiscore) s.hiscore = s.score;
}

void PowerUpSystem::updateLife(int value) {
    engine_.assets.playSfx(value >= 0 ? "NEW_LIFE" : "PLAYER_DEATH");
    engine_.state.player.lives += value != 0 ? value : 1;
}

bool PowerUpSystem::isSuperWeapon() const {
    int w = engine_.state.player.weapon;
    return w == 3 || w == 4 || w == 6 || w == 8 || w == 9 || w == 11;
}
