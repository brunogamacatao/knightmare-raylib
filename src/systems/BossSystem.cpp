#include "BossSystem.h"

#include <algorithm>
#include "../Engine.h"
#include "../Random.h"
#include "../Trig.h"

using namespace Constants;

void BossSystem::activateBoss() {
    GameState& s = engine_.state;
    if (s.boss.status == BossState::NOT_SPAWNED) return; // spawnBoss() never ran (should not happen: allocateObject can no longer fail)
    GameObj* body = s.boss.body;
    GameObj* shadow = body->shadowRef;
    s.boss.status = BossState::ACTIVE;
    if (shadow != nullptr) {
        shadow->y = body->y;
        shadow->gpr3 = 1; // fixed height
    }
    for (int i = 2; i <= 7; i++) activateHitBox(i, true);

    switch (s.stage) {
        case 1:
        case 8:
            if (s.stage == 8) s.animTick = 0;
            engine_.actionQueueSystem.enqueue(1, 5, 0, 0, 0, 3000);
            engine_.actionQueueSystem.enqueue(1, 5, 0, 0, 0, 6500);
            engine_.actionQueueSystem.enqueue(1, 5, 0, 0, 0, 9500);
            break;
        case 3:
            activateHitBox(2, false);
            body->shadowRef->gpr3 = 1;
            break;
        case 5:
            activateShield(true);
            activateHitBox(2, false);
            break;
        case 7:
            showBossBody(body, 0);
            engine_.actionQueueSystem.enqueue(1, 4, SCREEN_WIDTH / 2.0f - TILE_SIZE, TILE_SIZE * 5, 0, 4000);
            engine_.actionQueueSystem.enqueue(1, 4, SCREEN_WIDTH / 2.0f - TILE_SIZE, TILE_SIZE * 5, 0, 8000);
            break;
    }
}

void BossSystem::bossEnemyDestroyed(int objId, float gpr1, float gpr2) {
    GameState& s = engine_.state;
    switch (s.stage) {
        case 1:
            engine_.actionQueueSystem.enqueue(1, 5, 0, 0, 0, (long long)(Rnd::value() * 6000 + 2000));
            break;
        case 7:
            if (s.enemiesCount < 2) {
                engine_.actionQueueSystem.enqueue(1, 4, SCREEN_WIDTH / 2.0f - TILE_SIZE, TILE_SIZE * 5, 0,
                    (long long)(Rnd::value() * 5000 + 2000));
            }
            break;
    }
}

bool BossSystem::hitBoss(GameObj& hit) {
    GameState& s = engine_.state;
    if (s.boss.status < 2) return false;

    GameObj* body = s.boss.body;

    switch (s.stage) {
        case 3:
            if (!s.boss.hitBox[3]->hidden) {
                engine_.assets.playSfx("BOSS_SHIELD");
                return true;
            }
            break;
        case 5:
            if (body->aux1 == 0) { // helmet closed
                engine_.assets.playSfx("BOSS_SHIELD");
                return true;
            }
            break;
    }

    if (hit.gpr1 > 1000) return false; // invulnerable

    if (hit.gpr1 > 0 && hit.gpr1 < 1000) {
        hit.gpr1 += engine_.powerUpSystem.isSuperWeapon() ? -2 : -1;
        if (hit.gpr1 < 0) hit.gpr1 = 0;
        if (s.stage == 8) engine_.objectSystem.animateObjects(true);
    }
    float hitBoxLife = hit.gpr1;

    if (hitBoxLife <= 0) {
        body->gpr1--;
        if (s.stage == 8) {
            engine_.objectSystem.killEnemy(hit, true);
            if (body->gpr1 == 1) {
                createBossEye(1);
                engine_.objectSystem.animateObjects(true);
            }
        }
    }

    int aux = BOSS_LIFE[s.stage] / 3;
    switch (s.stage) {
        case 2:
        case 4:
            if (aux != 0 && ((int)hitBoxLife) % aux == 0) {
                // gpr1 is unused by action 90 - it always replaces the current boss body's skin.
                engine_.actionQueueSystem.enqueue(1, 90, 0,
                    ((int)hitBoxLife) / aux <= 1 ? TILE_SIZE * 10 : TILE_SIZE * 5, 0, 0);
            }
            break;
        case 3:
            if (hitBoxLife == aux || hitBoxLife == aux * 2) s.boss.status++;
            break;
    }

    if (body->gpr1 <= 0) {
        killBoss();
    } else {
        engine_.assets.playSfx("BOSS_HIT");
    }
    return true;
}

void BossSystem::killBoss() {
    GameState& s = engine_.state;
    s.boss.status = 1;
    s.player.state = Player::PAUSE;
    engine_.powerUpSystem.incrementScore(10000);
    engine_.actionQueueSystem.clear();
    engine_.objectSystem.destroyEnemiesShots();
    engine_.objectSystem.killAllEnemies(false);
    animateBoss();
    engine_.assets.playSong("SILENCE");
    engine_.assets.playSfx("BOSS_KILL");
    engine_.actionQueueSystem.enqueue(30, 100, 0, 0, 0, 40);
}

void BossSystem::destroyBoss() {
    GameState& s = engine_.state;
    GameObj* body = s.boss.body;
    float x = body->x, y = body->y;

    engine_.objectSystem.destroyObject(*body);
    for (int i = 2; i <= 7; i++) {
        GameObj* box = s.boss.hitBox[i];
        if (box != nullptr) engine_.objectSystem.destroyObject(*box);
        s.boss.hitBox[i] = nullptr;
    }
    s.boss.status = 0;
    // Not nulled in the original Java port (there it stays a harmless stale reference to a
    // GC-tracked, zeroed-out object until spawnBoss() reassigns it) - here the entity is actually
    // deallocated once GameState's sweep runs, so this is reset defensively. Nothing reads
    // boss.body while status == 0 (every caller gates on status first), so this changes no
    // observable behaviour.
    s.boss.body = nullptr;
    s.blinkActive = false;

    engine_.actionQueueSystem.enqueue(1, 34, x + TILE_SIZE * 2, y + TILE_SIZE, 1, 0);
    engine_.actionQueueSystem.enqueue(1, 34, x, y + TILE_SIZE * 2, 1, 0);
    engine_.actionQueueSystem.enqueue(1, 34, x + TILE_SIZE * 4, y + TILE_SIZE * 3, 1, 0);
    engine_.actionQueueSystem.enqueue(1, 34, x + TILE_SIZE * 5, y + TILE_SIZE * 4, 1, 0);
    engine_.actionQueueSystem.enqueue(1, 34, x + TILE_SIZE, y + TILE_SIZE * 5, 1, 0);
    engine_.actionQueueSystem.enqueue(1, 101, 0, 0, 0, 1500);
}

void BossSystem::spawnBoss() {
    GameState& s = engine_.state;
    float x = SCREEN_WIDTH / 2.0f - TILE_SIZE * 2.5f;
    // The object pool is unbounded (see GameState), so this can no longer fail the way it used to
    // when the fixed-size pool was already full right as the boss needed a slot.
    GameObj* body = &engine_.objectSystem.allocateObject(50, x, 0, 1, 0, TILE_SIZE * 14);

    s.boss.status = BossState::WAITING;
    s.boss.body = body;
    for (int i = 2; i <= 7; i++) s.boss.hitBox[i] = nullptr;
    body->gpr2 = 0; body->gpr3 = 0;

    switch (s.stage) {
        case 1:
            body->y = TILE_SIZE * (s.row == 0 ? 6 : -3);
            body->gpr2 = SCREEN_CENTER_W - TILE_SIZE_X2;
            createBossHitBox(2, TILE_SIZE_X2, TILE_SIZE_X2, 0, 0);
            showBossBody(body, 0);
            break;
        case 2:
            body->y = TILE_SIZE * (s.row == 0 ? 5 : -4);
            engine_.objectSystem.spawnShadow(*body, 0, TILE_SIZE * 6, false, 37);
            createBossHitBox(2, TILE_SIZE_X2, TILE_SIZE_X2, 0, 0);
            showBossBody(body, 0);
            break;
        case 3:
            body->y = TILE_SIZE * (s.row == 0 ? 6 : -3);
            body->gpr2 = 45;
            engine_.objectSystem.spawnShadow(*body, TILE_SIZE + TILE_SIZE / 2.0f, TILE_SIZE * 7, false, 0);
            createBossShield(TILE_SIZE, TILE_SIZE, TILE_SIZE * 3);
            createBossHitBox(2, TILE_SIZE_X2, TILE_SIZE_X2, 0, 0);
            showBossBody(body, 0);
            break;
        case 4:
            body->y = TILE_SIZE * (s.row == 0 ? 5 : -4);
            body->gpr2 = 45;
            engine_.objectSystem.spawnShadow(*body, 0, TILE_SIZE * 8, false, 37);
            showBossBody(body, 0);
            createBossHitBox(2, TILE_SIZE_X2, TILE_SIZE_X2, 0, 0);
            break;
        case 5:
            body->y = TILE_SIZE * (s.row == 0 ? 6 : -3);
            body->gpr3 = s.player.x < SCREEN_WIDTH ? -120 : 120;
            createBossShield(TILE_SIZE, TILE_SIZE, TILE_SIZE * 3);
            showBossBody(body, 0);
            createBossHitBox(2, TILE_SIZE_X2, TILE_SIZE_X2, 0, 0);
            break;
        case 6:
            body->y = TILE_SIZE * (s.row == 0 ? 6 : -3);
            body->gpr3 = TILE_SIZE * 6 * (s.player.x < 0 ? -1 : 1);
            showBossBody(body, 0);
            createBossHitBox(2, TILE_SIZE_X2, TILE_SIZE_X2, 0, 0);
            break;
        case 7:
            body->x = SCREEN_WIDTH / 2.0f - TILE_SIZE;
            body->y = TILE_SIZE * 6;
            createBossHitBox(2, 0, 0, TILE_SIZE_X2, 0);
            break;
        case 8:
            body->y = TILE_SIZE * (s.row == 0 ? 6 : -3);
            body->gpr1 = 6;
            body->gpr3 = TILE_SIZE * 6 * (s.player.x < 0 ? -1 : 1);
            showBossBody(body, 0);
            for (int i = 2; i <= 7; i++) createBossEye(i);
            break;
    }
}

void BossSystem::createBossHitBox(int ix, float offsetX, float offsetY, float width, float life) {
    GameState& s = engine_.state;
    if (life == 0) life = (float)BOSS_LIFE[s.stage];
    life *= BOSS_DIFFICULTY[s.difficulty];
    if (width == 0) width = TILE_SIZE;

    GameObj* body = s.boss.body;
    GameObj& hitBox = engine_.objectSystem.allocateObject(60, body->x + offsetX, body->y + offsetY, life, offsetX, offsetY);
    s.boss.hitBox[ix] = &hitBox;
    hitBox.hitW = width;
    hitBox.hitH = TILE_SIZE;
    hitBox.hidden = false;
    hitBox.y = std::max(-7.0f, hitBox.y);
}

void BossSystem::createBossShield(float offsetX, float offsetY, float width) {
    createBossHitBox(3, offsetX, offsetY, width, 1000000);
}

void BossSystem::showBossBody(GameObj* body, int tileOffset) {
    GameState& s = engine_.state;
    int tx = 0, ty = 0, tw = TILE_SIZE * 5, th = TILE_SIZE * 5;
    switch (s.stage) {
        case 2: tx = 80; break;
        case 3: tx = 200; break;
        case 4: ty = 80; th = TILE_SIZE * 6; break;
        case 5: tx = 120; ty = 80; th = TILE_SIZE * 6; break;
        case 6: ty = 128; th = TILE_SIZE * 6; break;
        case 7: tx = 0; ty = 56; tw = TILE_SIZE_X2; th = TILE_SIZE_X2; break;
        case 8: tx = 80; ty = 128; tw = TILE_SIZE * 6; th = TILE_SIZE * 6; break;
    }
    body->region = engine_.assets.boss(tx + tileOffset, ty, tw, th);
    body->layer = 2;
    body->hidden = false;
}

void BossSystem::createBossEye(int eyeId) {
    GameState& s = engine_.state;
    float offsetX, offsetY;
    switch (eyeId) {
        case 1: offsetX = TILE_SIZE_X2; offsetY = 0; break;
        case 2: offsetX = 3; offsetY = 6; break;
        case 3: offsetX = TILE_SIZE * 3 + 5; offsetY = 6; break;
        case 4: offsetX = TILE_SIZE_X2; offsetY = TILE_SIZE_X2; break;
        case 5: offsetX = 3; offsetY = TILE_SIZE_X2 + 6; break;
        case 6: offsetX = TILE_SIZE * 3 + 5; offsetY = TILE_SIZE_X2 + 6; break;
        default: return;
    }
    GameObj* body = s.boss.body;
    GameObj& eye = engine_.objectSystem.spawnObject(50, body->x + offsetX, body->y + offsetY, 0);
    s.boss.hitBox[eyeId + 1] = &eye;
    eye.gpr1 = BOSS_LIFE[8] * BOSS_DIFFICULTY[s.difficulty];
    eye.gpr2 = offsetX;
    eye.gpr3 = offsetY;
}

void BossSystem::bossFire() {
    GameState& s = engine_.state;
    long long delay;
    bool disableRnd = false;
    switch (s.stage) {
        case 1: case 6: delay = 70; break;
        case 2: case 4: delay = 120; disableRnd = true; break;
        case 3: return;
        case 5: delay = 70; disableRnd = true; break;
        case 7: delay = 70; break;
        case 8:
            for (int i = 2; i <= 7; i++) {
                GameObj* eye = s.boss.hitBox[i];
                if (eye == nullptr) continue;
                float fireRate = 130 + 10 * i;
                if (i == 2) fireRate /= 4;
                if (eye->id > 0 && s.tick % (int)fireRate == 0) {
                    if (eye->gpr1 <= 0 || eye->gpr1 > 1000) continue;
                    engine_.objectSystem.enemyFire(*eye, false);
                }
            }
            return;
        default: return;
    }
    if (delay > 0 && s.tick % delay == 0) engine_.objectSystem.enemyFire(*s.boss.body, disableRnd);
}

void BossSystem::animateBoss() {
    GameState& s = engine_.state;
    GameObj* body = s.boss.body;
    int x = 0, y = 0;
    switch (s.stage) {
        case 1:
            if (s.animTick % 8 > 3 && s.boss.status != 1) x = TILE_SIZE * 5;
            break;
        case 3: {
            int tick = s.animTick % 48;
            x = s.boss.status == 3 ? 80 : 200;
            y = s.boss.status == 2 ? 0 : 40;
            if (tick > 23) {
                if (tick == 24) { activateHitBox(2, true); activateHitBox(3, false); }
                else if (tick == 34) { engine_.objectSystem.enemyFire(*body, true); }
                int t2 = s.animTick % 4;
                x += (t2 < 2 || s.boss.status == 1) ? TILE_SIZE * 5 : TILE_SIZE * 10;
            } else if (tick == 0) {
                activateHitBox(2, false);
                activateHitBox(3, true);
            }
            break;
        }
        case 5:
            x = 120; y = 80;
            if (s.animTick % 10 > 4 && s.boss.status != 1) x += TILE_SIZE * 5;
            if (body->gpr3 < 0) x += TILE_SIZE * 10;
            break;
        case 6:
            y = 128;
            if (s.animTick % 12 > 5 && s.boss.status != 1) x = TILE_SIZE * 5;
            break;
        case 7: {
            float hitBoxLife = s.boss.hitBox[2]->gpr1;
            if (s.boss.status != 1 && s.animTick % 4 < 2) {
                if (hitBoxLife < 11) x = TILE_SIZE_X4;
                else if (hitBoxLife < 21) x = TILE_SIZE_X2;
            }
            y = 56;
            body->region = engine_.assets.boss(x, y, body->region.w, body->region.h);
            body->flipX = s.animTick % 6 <= 2;
            return;
        }
        case 8:
            x = 80; y = 128;
            if (s.boss.status != 1 && (int)body->gpr1 != (int)body->gpr3) {
                if (s.animTick % 12 > 5) x += TILE_SIZE * 6;
            }
            break;
        default:
            return;
    }
    int w = body->region.valid() ? body->region.w : TILE_SIZE * 5;
    int h = body->region.valid() ? body->region.h : TILE_SIZE * 5;
    body->region = engine_.assets.boss(x, y, w, h);
}

void BossSystem::scrollBoss() {
    if (engine_.state.stage == 7) return;
    GameObj* body = engine_.state.boss.body;
    body->y += 1;
    moveHitBox();
}

void BossSystem::moveBoss() {
    GameState& s = engine_.state;
    if (s.boss.status < 2) return;
    GameObj* body = s.boss.body;
    float rightEdge = EDGE_RIGHT - TILE_SIZE * 5;

    switch (s.stage) {
        case 1: case 8: {
            body->gpr2++;
            if ((int)body->x == (int)body->gpr3) {
                if (((int)body->gpr2) % 150 == 0) {
                    body->gpr3 = std::min(std::max(s.player.x - TILE_SIZE_X2, EDGE_LEFT), rightEdge);
                }
            } else {
                body->x += (body->x > body->gpr3 ? -1 : 1) * BOSS_SPEED * s.deltaTime;
            }
            break;
        }
        case 2: case 3: case 4: {
            float speedX, speedY, movHeight, y;
            if (s.stage == 2) { speedX = 1.2f; speedY = 9; movHeight = 6; y = TILE_SIZE * 5; }
            else { speedX = 1.8f; speedY = 3.6f; movHeight = 12; y = TILE_SIZE * 6; }
            float iniPos = (SCREEN_WIDTH - TILE_SIZE * 6) / 2.0f;
            body->gpr2 += BOSS_SPEED * s.deltaTime * speedX;
            body->gpr3 += BOSS_SPEED * s.deltaTime * speedY;
            body->x = iniPos + (iniPos - TILE_SIZE_X4) * Trig::sin(body->gpr2);
            body->y = y + movHeight * Trig::cos(body->gpr3);
            break;
        }
        case 5: {
            body->gpr2++;
            if (((int)body->gpr2) % 200 == 0) {
                body->gpr3 = body->x > s.player.x ? -120 : 120;
                if ((int)body->x == (int)EDGE_LEFT && body->gpr3 < 0) body->gpr3 = -body->gpr3;
                else if ((int)body->x == (int)rightEdge && body->gpr3 > 0) body->gpr3 = -body->gpr3;
                body->aux1 = 0;
                activateHitBox(2, false);
                activateShield(true);
            }
            if (body->aux1 == 0 && ((int)body->gpr2) % 300 == 0) {
                body->aux1 = -1;
                activateHitBox(2, true);
                activateShield(false);
                engine_.assets.playSfx("HELMET");
            }
            if (body->gpr3 != 0) {
                float speedX = body->gpr3 < 0 ? 1.0f : -1.0f;
                body->gpr3 += speedX;
                body->x += BOSS_SPEED * s.deltaTime * -speedX;
                body->x = std::min(std::max(body->x, EDGE_LEFT), rightEdge);
            }
            break;
        }
        case 6: {
            body->gpr2++;
            if ((int)body->x == (int)body->gpr3) {
                if (((int)body->gpr2) % 300 == 0) {
                    engine_.assets.playSfx("TERRAIN");
                    engine_.actionQueueSystem.enqueue(1, 49, body->x + TILE_SIZE_X2, body->y + TILE_SIZE * 6, 0, 0);
                    float speedX = body->x > s.player.x ? -1.0f : 1.0f;
                    if ((int)body->x == (int)EDGE_LEFT) speedX = 1;
                    if ((int)body->x == (int)rightEdge) speedX = -1;
                    body->gpr3 = std::min(std::max(body->x + TILE_SIZE * 9 * speedX, EDGE_LEFT), rightEdge);
                }
            } else {
                body->x += (body->x > body->gpr3 ? -1 : 1) * BOSS_SPEED * s.deltaTime;
            }
            break;
        }
        case 7: {
            float speedX = 30 + BOSS_LIFE[7] - s.boss.hitBox[2]->gpr1;
            if (body->gpr2 == 0 || ((int)body->gpr2) % 200 == 0 || body->x < TILE_SIZE_X4 || body->x > SCREEN_WIDTH - TILE_SIZE * 6) {
                body->gpr3 = body->velocityAngleDeg() - 90;
            }
            float oldX = body->x, oldY = body->y;
            body->gpr2++;
            body->x += speedX * Trig::cos(body->gpr3) * s.deltaTime;
            body->y += speedX * Trig::sin(body->gpr3) * s.deltaTime;
            body->prevX = oldX; body->prevY = oldY; body->hasPrev = true;
            if (body->y > SCREEN_HEIGHT + 1) {
                body->x = SCREEN_WIDTH / 2.0f - TILE_SIZE;
                body->y = TILE_SIZE * 5;
            }
            break;
        }
    }

    moveHitBox();
}

void BossSystem::activateHitBox(int i, bool active) {
    GameState& s = engine_.state;
    if (s.boss.hitBox[i] == nullptr) return;
    s.boss.hitBox[i]->hidden = !active;
}

void BossSystem::activateShield(bool active) { activateHitBox(3, active); }

bool BossSystem::isHitBoxActive(int i) const { return !engine_.state.boss.hitBox[i]->hidden; }

void BossSystem::moveHitBox() {
    GameState& s = engine_.state;
    GameObj* body = s.boss.body;
    for (int i = 2; i <= 7; i++) {
        GameObj* box = s.boss.hitBox[i];
        if (box == nullptr) continue;
        box->x = body->x + box->gpr2;
        box->y = body->y + box->gpr3;
        if (!box->hidden) box->y = std::max(-7.0f, box->y);
    }
}
