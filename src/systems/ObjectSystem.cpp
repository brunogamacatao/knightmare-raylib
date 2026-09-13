#include "ObjectSystem.h"

#include <algorithm>
#include <cmath>
#include "../Engine.h"
#include "../Random.h"
#include "../Trig.h"

using namespace Constants;

namespace {
std::unique_ptr<Shot> makeShot() {
    auto sh = std::make_unique<Shot>();
    sh->seq = nextShotSeq();
    return sh;
}
} // namespace

// ---------------------------------------------------------------- slots

Shot& ObjectSystem::getFreeShotSlot() {
    auto sh = makeShot();
    Shot& ref = *sh;
    engine_.state.enemyShots.push_back(std::move(sh));
    return ref;
}

// ---------------------------------------------------------------- input / player movement

void ObjectSystem::processInput() {
    GameState& s = engine_.state;
    Player& p = s.player;
    if (p.state != Player::PLAYING) return;
    p.isMoving = false;

    bool fireHeld = engine_.input != nullptr && engine_.input->isFireHeld();
    if (fireHeld) {
        if (!p.fireDownLastFrame) fire();
        p.fireDownLastFrame = true;
    } else {
        p.fireDownLastFrame = false;
    }

    bool left = engine_.input != nullptr && engine_.input->isLeft();
    bool right = engine_.input != nullptr && engine_.input->isRight();
    bool up = engine_.input != nullptr && engine_.input->isUp();
    bool down = engine_.input != nullptr && engine_.input->isDown();

    if (left) movePlayer(KB_LEFT); else if (right) movePlayer(KB_RIGHT);
    if (up) movePlayer(KB_UP); else if (down) movePlayer(KB_DOWN);
}

void ObjectSystem::movePlayer(int direction) {
    GameState& s = engine_.state;
    Player& p = s.player;
    if (p.state == Player::DEAD) return;

    float x = p.x, y = p.y;
    float speed = p.effectiveSpeed();
    switch (direction) {
        case KB_LEFT:
            p.x -= speed * s.deltaTime;
            if (engine_.mapSystem.mapCollide(p.x, p.y)) p.x = x;
            if (p.x < 0) p.x = SCREEN_WIDTH - TILE_SIZE * 2;
            p.isMoving = true;
            engine_.mapSystem.checkScrollCollision();
            break;
        case KB_RIGHT:
            p.x += speed * s.deltaTime;
            if (engine_.mapSystem.mapCollide(p.x, p.y)) p.x = x;
            if (p.x > SCREEN_WIDTH - TILE_SIZE * 2) p.x = 0;
            p.isMoving = true;
            engine_.mapSystem.checkScrollCollision();
            break;
        case KB_UP: {
            p.y -= speed * s.deltaTime;
            if (engine_.mapSystem.mapCollide(p.x, p.y)) p.y = y;
            float minY = TILE_SIZE * 6;
            if (p.y < minY) p.y = minY;
            p.isMoving = true;
            engine_.mapSystem.checkScrollCollision();
            break;
        }
        case KB_DOWN: {
            p.y += speed * s.deltaTime;
            if (engine_.mapSystem.mapCollide(p.x, p.y)) p.y = y;
            float maxY = s.visibleBottomY();
            if (p.y > maxY) p.y = maxY;
            p.isMoving = true;
            engine_.mapSystem.checkScrollCollision();
            break;
        }
    }
}

void ObjectSystem::autoMovePlayerToPortal() {
    GameState& s = engine_.state;
    float y = s.player.y > TILE_SIZE * 6 ? TILE_SIZE * 6 : TILE_SIZE;
    movePlayerTo(SCREEN_WIDTH / 2.0f - TILE_SIZE, y);
    if ((int)s.player.y == TILE_SIZE) {
        s.player.state = Player::READY_NEXT_STAGE;
    }
}

void ObjectSystem::movePlayerTo(float x, float y) {
    GameState& s = engine_.state;
    Player& p = s.player;
    float ang = Trig::atan2(y - p.y + TILE_SIZE, x - p.x);
    p.x += PLAYER_INIT_SPEED * s.deltaTime * Trig::cos(ang) * 1.5f;
    if ((int)p.y >= (int)y) {
        p.y += -PLAYER_INIT_SPEED * s.deltaTime;
    }
}

void ObjectSystem::movePlayerToPortal() {
    createPortal();
    engine_.state.player.state = Player::MOVING_TO_PORTAL;
    engine_.assets.playSfx("START_STAGE");
}

// ---------------------------------------------------------------- animation

void ObjectSystem::animatePlayer() {
    GameState& s = engine_.state;
    Player& p = s.player;
    float speed = p.powerUp == Player::PWR_INVINCIBLE ? PLAYER_MAX_SPEED : p.speed;
    int offset = 0;

    if (p.state == Player::CUTSCENE_STILL) {
        return;
    }
    if (p.state > 8) {
        p.state++;
    } else if (p.state == Player::DEAD) {
        int idx = (int)std::min<float>(p.animCounter, (float)(PLAYER_DEATH_ANIM.size() - 1));
        offset = PLAYER_DEATH_ANIM[idx] * TILE_SIZE;
        p.animCounter += 0.5f;
        if (p.animCounter > (PLAYER_DEATH_ANIM.size() - 1) * 1.5f) p.state = Player::RESTART_STAGE;
    } else {
        p.animCounter += p.isMoving ? 2 + speed / PLAYER_INIT_SPEED : 1;
        if (p.powerUp == Player::PWR_SHIELD) {
            // shield sprite kept visible, handled by renderer
        } else if (p.powerUp > 1) {
            if (s.powerUpTimer > 10 || (s.powerUpTimer - (int)s.powerUpTimer) > 0.5f) {
                offset = TILE_SIZE_X2 * p.powerUp;
            }
        }
    }
    p.animOffsetX = offset;
}

void ObjectSystem::animateObjects(bool reload) {
    GameState& s = engine_.state;
    bool sfxPlayedThisCall = false;
    size_t n = s.obj.size();
    for (size_t oi = 0; oi < n; oi++) {
        GameObj& o = *s.obj[oi];
        int objId = o.id;
        if (objId == 0 || objId == 39 || objId > 50) continue;
        if (objId < 20 && s.freezeTimer >= 0) continue;

        int effect = -1;
        int offsetX = 0, offsetY = 0, offsetH = 0;

        switch (objId) {
            case 4: case 9: case 10: case 15:
                effect = (s.animTick % 6 > 2) ? 1 : 0;
                break;
            case 7:
                if (o.gpr2 > 1) continue;
                if (s.animTick % 6 > 2) offsetX = TILE_SIZE_X2;
                break;
            case 8:
                if (o.gpr3 <= 0) { objId = 5; } else { effect = (s.animTick % 6 > 2) ? 1 : 0; }
                break;
            case 11:
                continue;
            case 12:
                break;
            case 16: {
                float life = o.gpr3;
                if (life > 40) life = 0;
                else if (life > 32) offsetX = (((int)life) % 2 != 0) ? TILE_SIZE_X2 : 0;
                else if (life > 8) offsetX = TILE_SIZE_X2;
                else if (life > 0) offsetX = (((int)life) % 2 != 0) ? TILE_SIZE_X2 : 0;
                if (life > 0) life++;
                if (s.fire && life == 0) life = 1;
                o.gpr3 = life;
                if (!sfxPlayedThisCall && s.animTick % 20 == 0) { engine_.assets.playSfx("RED_DEATH"); sfxPlayedThisCall = true; }
                if (o.gpr3 == 0) continue;
                break;
            }
            case 20:
                offsetX = WEAPON_ANIM[(int)o.gpr1][(int)std::max(0.0f, o.gpr2 - 2)] * TILE_SIZE_X2;
                if (o.gpr1 == WEAPON_ANIM.size() - 1) o.gpr1 = 0; else o.gpr1++;
                break;
            case 21:
                if (std::fmod(s.animTick, 1.5f) > 0) continue;
                offsetX = PUP_ANIM[(int)o.gpr1][(int)std::max(0.0f, o.gpr2 - 2)] * TILE_SIZE_X2;
                if (o.gpr1 == PUP_ANIM.size() - 1) o.gpr1 = 0; else o.gpr1++;
                break;
            case 34:
                if (o.gpr1 > (float)FIRE_ANIM.size() - 1) { destroyObject(o); continue; }
                offsetX = FIRE_ANIM[(int)o.gpr1] * TILE_SIZE_X2;
                o.gpr1++;
                break;
            case 49:
                if (std::fmod(s.animTick, 1.6f) > 0) continue;
                effect = -2;
                offsetY = (int)(-TILE_SIZE * o.gpr2);
                offsetH = (int)std::min<float>(TILE_SIZE * 12, std::abs(offsetY));
                o.gpr2++;
                if (o.gpr2 > 16) { if (o.gpr2 > 22) destroyObject(o); continue; }
                break;
            case 50: {
                if (s.stage != 8 || s.boss.status != 2 || &o == s.boss.body) continue;
                int eyeIx = 0;
                for (int i = 2; i <= 7; i++) if (s.boss.hitBox[i] == &o) { eyeIx = i - 1; break; }
                int auxTick = eyeIx - 15;
                int aux2 = s.animTick % (115 + auxTick);
                if (o.gpr1 == 0) {
                    offsetY = 0;
                } else if (aux2 < 3 + auxTick) {
                    offsetY = TILE_SIZE_X2;
                } else if (aux2 < 103 + auxTick) {
                    offsetY = TILE_SIZE_X4;
                    effect = ((aux2 / 8) % 2 == 0) ? 0 : 1;
                    if (aux2 == 4 + auxTick) effect = -2;
                    if (o.gpr1 > 1000) o.gpr1 -= 1000;
                } else if (aux2 < 112 + auxTick) {
                    offsetY = TILE_SIZE_X2;
                    if (o.gpr1 < 1000) o.gpr1 += 1000;
                } else {
                    offsetY = 0;
                }
                float life = o.gpr1 > 1000 ? o.gpr1 - 1000 : o.gpr1;
                offsetX = TILE_SIZE_X2 * (int)(5 - (life / (BOSS_LIFE[8] * BOSS_DIFFICULTY[s.difficulty] / 5)));
                if (reload) effect = -1;
                break;
            }
            default:
                if (s.animTick % 6 > 2) offsetX = TILE_SIZE_X2;
                break;
        }

        applyFrame(o, objId, effect, offsetX, offsetY, offsetH);
    }
}

void ObjectSystem::applyFrame(GameObj& o, int objId, int effect, int offsetX, int offsetY, int offsetH) {
    if (effect >= 0) {
        o.flipX = effect == 1;
    } else {
        const ObjTile& t = OBJ_TILE[objId];
        bool boss = objId == 50;
        o.region = boss ? engine_.assets.boss(t.x + offsetX, t.y + offsetY, t.w, t.h + offsetH)
                        : engine_.assets.obj(t.x + offsetX, t.y + offsetY, t.w, t.h + offsetH);
        o.flipX = false;
    }
}

void ObjectSystem::animateShots() {
    GameState& s = engine_.state;
    size_t np = s.playerShots.size();
    for (size_t i = 0; i < np; i++) { Shot& sh = *s.playerShots[i]; if (!sh.isFree()) animateShot(sh); }
    size_t ne = s.enemyShots.size();
    for (size_t i = 0; i < ne; i++) { Shot& sh = *s.enemyShots[i]; if (!sh.isFree()) animateShot(sh); }
    if (!s.contSfx.empty()) {
        engine_.assets.playSfx(s.contSfx);
    }
}

void ObjectSystem::animateShot(Shot& sh) {
    GameState& s = engine_.state;
    int rot = 0, tileId, offsetX = 0;
    switch (sh.weaponId) {
        case 4: case 9: {
            sh.gpr3++;
            int m = ((int)sh.gpr3) % 12;
            if (m == 9) { rot = 3; tileId = 44; }
            else if (m == 6) { rot = 3; tileId = 43; }
            else if (m == 3) { rot = 0; tileId = 44; }
            else if (m == 0) { rot = 0; tileId = 43; }
            else return;
            sh.tileId = tileId; sh.rot = rot; sh.offsetX = 0;
            setShotRegion(sh);
            return;
        }
        case 25: {
            sh.gpr3++;
            int m = ((int)sh.gpr3) % 4;
            if (m == 3) { rot = 2; tileId = 26; }
            else if (m == 2) { rot = 0; tileId = 27; }
            else if (m == 1) { rot = 0; tileId = 26; }
            else { rot = 0; tileId = 25; }
            sh.tileId = tileId; sh.rot = rot; sh.offsetX = 0;
            setShotRegion(sh);
            return;
        }
        case 28: {
            if (sh.gpr1 == 0 && sh.gpr2 == 0) {
                tileId = 35;
                sh.gpr3++;
                if (sh.gpr3 > 18) { destroyShot(sh); return; }
                else if (((int)sh.gpr3) % 6 > 2) offsetX = TILE_SIZE_X2;
            } else return;
            sh.tileId = tileId; sh.rot = 0; sh.offsetX = offsetX;
            setShotRegion(sh);
            return;
        }
        case 32: {
            sh.gpr3++;
            tileId = 32;
            int m = ((int)sh.gpr3) % 8;
            if (m == 6) { rot = 1; offsetX = TILE_SIZE_X2; }
            else if (m == 4) { rot = 0; offsetX = TILE_SIZE_X2; }
            else if (m == 2) { rot = 1; }
            else if (m == 0) { rot = 0; }
            else return;
            sh.tileId = tileId; sh.rot = rot; sh.offsetX = offsetX;
            setShotRegion(sh);
            return;
        }
        case 33: {
            tileId = 33; rot = 0;
            offsetX = TILE_SIZE_X2 * (s.animTick % 4);
            sh.tileId = tileId; sh.rot = rot; sh.offsetX = offsetX;
            setShotRegion(sh);
            return;
        }
        default:
            break;
    }
}

void ObjectSystem::setShotRegion(Shot& sh) {
    const ObjTile& t = OBJ_TILE[sh.tileId];
    sh.region = engine_.assets.obj(t.x + sh.offsetX, t.y, t.w, t.h);
}

void ObjectSystem::fadeMap(int start) {
    GameState& s = engine_.state;
    float delta = s.player.state - start;
    float vspeed = 2;
    if (delta > 60 / vspeed) {
        s.fadeShowText = true;
        return;
    }
    float hspeed = vspeed * 1.6f * delta;
    vspeed = vspeed * delta;
    s.fadeActive = true;
    s.fadeX = hspeed;
    s.fadeY = TILE_SIZE_X2 + vspeed;
    s.fadeW = SCREEN_WIDTH - hspeed * 2;
    s.fadeH = SCREEN_HEIGHT - vspeed * 2;
}

// ---------------------------------------------------------------- movement

void ObjectSystem::moveShots() {
    GameState& s = engine_.state;
    size_t np = s.playerShots.size();
    for (size_t i = 0; i < np; i++) { Shot& sh = *s.playerShots[i]; if (!sh.isFree()) moveShot(sh); }
    size_t ne = s.enemyShots.size();
    for (size_t i = 0; i < ne; i++) { Shot& sh = *s.enemyShots[i]; if (!sh.isFree()) moveShot(sh); }
}

void ObjectSystem::moveShot(Shot& sh) {
    GameState& s = engine_.state;
    float speedX = 0, speedY = 0;
    switch (sh.weaponId) {
        case 4: case 9:
            sh.x = s.player.x;
            sh.gpr2 += 2.5f;
            speedY = sh.gpr2;
            break;
        case 28: {
            speedX = sh.gpr1; speedY = sh.gpr2;
            float y = sh.gpr3;
            if (y > 0 && speedY > 0 && sh.y >= y) {
                sh.gpr1 = 0; speedX = 0;
                sh.gpr2 = 0; speedY = 0;
                sh.gpr3 = 0;
            }
            break;
        }
        case 33:
            sh.gpr3 += (sh.seq % 2 != 0) ? 1 : -1;
            if (std::abs(sh.gpr3) > 720) { destroyShot(sh); return; }
            sh.x = sh.gpr1 + TILE_SIZE * 7 * Trig::sin(sh.gpr3);
            sh.y = sh.gpr2 + TILE_SIZE * 7 * -Trig::cos(sh.gpr3);
            break;
        default:
            speedX = sh.gpr1; speedY = sh.gpr2;
            break;
    }

    if (sh.weaponId != 33) {
        sh.x += speedX * s.deltaTime;
        sh.y += speedY * s.deltaTime;
    }

    float topEdge = 0;
    if (sh.weaponId == 4 || sh.weaponId == 9) {
        if (sh.y < topEdge) sh.y = topEdge;
    }

    if (sh.y < topEdge || sh.y > s.visibleBottomY() + TILE_SIZE_X2 || sh.x < 0 || sh.x > SCREEN_WIDTH) {
        destroyShot(sh);
    }
}

void ObjectSystem::moveAndProcessObjects() {
    GameState& s = engine_.state;
    size_t n = s.obj.size();
    for (size_t oi = 0; oi < n; oi++) {
        GameObj& o = *s.obj[oi];
        int objId = o.id;
        if (objId == 0 || objId > (int)OBJ_TILE.size() - 1) continue;
        // id 60 (portal / boss hit boxes) has no OBJ_TILE entry - its position is driven entirely
        // by BossSystem::moveHitBox()/createPortal(), not this generic movement pass. (60 is also
        // handled explicitly by its own `case 60: continue;` below.)
        if (objId != 60 && OBJ_TILE[objId].w == 0 && OBJ_TILE[objId].h == 0) continue;
        if (s.freezeTimer >= 0 && objId != 20 && objId != 21) continue;

        float oldX = o.x, oldY = o.y;
        int dataId = OBJ_TILE[objId].dataId;
        float offsetX = 0, offsetY = 0;
        int flip = -1;

        switch (objId) {
            case 2: case 3: case 14: {
                float direction;
                if (objId == 2 || objId == 14) {
                    if (o.gpr3 == RIGHT) {
                        direction = SCREEN_CENTER_W + (objId == 2 ? 0 : TILE_SIZE * 6);
                        o.x = SCREEN_WIDTH + direction * Trig::cos(o.gpr2);
                    } else {
                        direction = SCREEN_CENTER_W + (objId == 2 ? TILE_SIZE : TILE_SIZE * 6);
                        o.x = direction * Trig::cos(o.gpr2);
                    }
                } else {
                    o.x = (SCREEN_CENTER_W - TILE_SIZE) + (SCREEN_CENTER_W - TILE_SIZE_X4) * Trig::cos(o.gpr2);
                }
                o.gpr2 += OBJ_DATA[dataId][0] * s.deltaTime;
                o.y += OBJ_DATA[dataId][1] * s.deltaTime;
                if (s.animTick % 6 < 3) offsetY = OBJ_TILE[objId].h / 2.0f;
                break;
            }
            case 4: {
                if (s.row <= 0) {
                    if (o.gpr3 == 0) {
                        if (s.player.y - o.y < KNIGHT_CHANGE_DIRECT_DIST_PX) {
                            o.gpr2 = o.velocityAngleDeg() - 90;
                            o.gpr3 = 1;
                        } else {
                            o.gpr2 = 90;
                        }
                    }
                    float speed = o.gpr3 != 0 ? 60.0f : 30.0f;
                    o.x += speed * Trig::cos(o.gpr2) * s.deltaTime;
                    o.y += speed * Trig::sin(o.gpr2) * s.deltaTime;
                } else {
                    if (o.gpr2 == 0 && s.player.y - o.y < KNIGHT_CHANGE_DIRECT_DIST_PX) {
                        o.gpr2 = o.x;
                        o.gpr3 = o.x > SCREEN_CENTER_W ? LEFT : RIGHT;
                    } else if (o.gpr3 != 0 && std::abs(o.gpr2 - o.x) > KNIGHT_MAX_HORIZONTAL_DIST_PX) {
                        o.gpr3 = 0;
                    }
                    if (o.gpr3 != 0) {
                        o.x += OBJ_DATA[4][0] * s.deltaTime * o.gpr3;
                    } else {
                        o.y += OBJ_DATA[4][1] * s.deltaTime;
                    }
                }
                break;
            }
            case 5: {
                if (o.gpr1 == 1) {
                    o.x += o.gpr2 * s.deltaTime;
                    o.y += o.gpr3 * s.deltaTime;
                } else {
                    if (o.y > SCREEN_HEIGHT / 2.0f + TILE_SIZE_X2) {
                        if (o.x <= 0 || o.x >= SCREEN_WIDTH - TILE_SIZE_X2) { destroyObject(o); continue; }
                    } else if (o.gpr3 == LEFT && o.x <= TILE_SIZE_X2) {
                        o.gpr3 = RIGHT;
                    } else if (o.gpr3 == RIGHT && o.x >= SCREEN_WIDTH - TILE_SIZE_X4) {
                        o.gpr3 = LEFT;
                    }
                    o.x += OBJ_DATA[dataId][0] * s.deltaTime * o.gpr3;

                    if (o.gpr2 < 0) {
                        o.gpr2 += 10 * s.deltaTime;
                        o.y += 10 * s.deltaTime;
                    } else {
                        o.y += -TILE_SIZE / 2.0f * Trig::sin(o.gpr2);
                        o.gpr2 += 1;
                        o.y += TILE_SIZE / 2.0f * Trig::sin(o.gpr2);
                        if (std::fmod(o.gpr2, OBJ_DATA[5][1]) == 0) o.gpr2 = -TILE_SIZE - TILE_SIZE / 2.0f;
                    }
                }
                break;
            }
            case 6: {
                if (o.gpr2 == 0 && s.player.y - o.y < DEMON_CHANGE_DIRECT_DIST_PX) {
                    o.gpr2 = o.x > s.player.x ? LEFT : RIGHT;
                }
                o.x += OBJ_DATA[dataId][0] * s.deltaTime * o.gpr2;
                o.y += OBJ_DATA[dataId][1] * s.deltaTime;
                break;
            }
            case 7: {
                if (o.gpr2 > 1) {
                    o.gpr3 += s.deltaTime;
                    offsetY = -TILE_SIZE;
                    float speed;
                    if (o.gpr3 > 3.2f) { destroyObject(o); continue; }
                    else if (o.gpr3 > 3) speed = TILE_SIZE;
                    else if (o.gpr3 > 2.8f) speed = TILE_SIZE * 2;
                    else if (o.gpr3 > 2.6f) {
                        if (o.gpr1 == 10) { engine_.assets.playSfx("SKELETON_RESTORED"); o.gpr1 = 11; }
                        speed = TILE_SIZE * 3;
                    } else if (o.gpr3 > 0.6f) { speed = TILE_SIZE_X4; offsetY = 0; }
                    else if (o.gpr3 > 0.4f) speed = TILE_SIZE * 3;
                    else if (o.gpr3 > 0.2f) speed = TILE_SIZE_X2;
                    else { speed = TILE_SIZE; if (o.gpr1 != 10) o.gpr1 = 10; }
                    offsetX = (int)(speed * (o.gpr2 == 2 ? -1 : 1));
                } else if (o.gpr3 == 0) {
                    float speed = OBJ_DATA[dataId][0] + OBJ_DATA[dataId][0] / 3 * (OBJ_DATA[dataId][2] - o.gpr1 + 1);
                    o.x += speed * s.deltaTime * o.gpr2;
                    o.y += OBJ_DATA[dataId][1] * s.deltaTime;
                    if (o.x < TILE_SIZE_X4) o.gpr2 = RIGHT;
                    else if (o.x > SCREEN_WIDTH - TILE_SIZE * 6) o.gpr2 = LEFT;
                } else {
                    o.gpr3 += s.deltaTime;
                    if (o.gpr3 > 4.2f) {
                        o.x += offsetX; o.y += offsetY;
                        o.gpr3 = 0;
                    }
                }
                break;
            }
            case 8: {
                if (o.gpr3 == 0 && s.player.y - o.y < DEMON_ATTACK_DIST_PX) {
                    o.gpr3 = 1;
                    replaceSpriteSkin(o, objId, o.x, o.y);
                }
                if (o.gpr3 <= 0) {
                    float direction = o.gpr3 < 0 ? -1.0f : 1.0f;
                    o.x += o.gpr2 * s.deltaTime;
                    o.y += OBJ_DATA[dataId][1] * s.deltaTime * direction;
                } else {
                    offsetY = (int)(TILE_SIZE / 2.0f);
                    o.gpr3 += s.deltaTime;
                    if (o.gpr3 > 2) {
                        o.gpr3 = -1;
                        enemyFire(o, false);
                        replaceSpriteSkin(o, 5, o.x, o.y);
                    }
                }
                break;
            }
            case 9: {
                if ((o.y < SCREEN_CENTER_H + TILE_SIZE && o.gpr2 <= 180) || o.gpr2 == 999) {
                    o.y += OBJ_DATA[dataId][1] * s.deltaTime * (o.gpr2 == 999 ? -1 : 1);
                } else {
                    float speed = o.gpr3 == LEFT ? -1.0f : 1.0f;
                    float aux;
                    if (speed > 0) aux = TILE_SIZE * 9 + Trig::cos(o.gpr2 * 0.666f) * TILE_SIZE_X2;
                    else aux = TILE_SIZE * 9 - Trig::cos((o.gpr2 + 90) * 0.666f) * TILE_SIZE_X2;
                    o.x = SCREEN_CENTER_W - TILE_SIZE + aux * Trig::cos(o.gpr2 * speed);
                    o.y = SCREEN_CENTER_H + TILE_SIZE + (SCREEN_CENTER_H - TILE_SIZE * 3) * Trig::sin(o.gpr2 * speed);
                    o.gpr2 += 0.6f;
                    if ((speed > 0 && o.gpr2 >= 540) || (speed < 0 && o.gpr2 >= 720)) o.gpr2 = 999;
                }
                break;
            }
            case 10: {
                if (std::abs(o.gpr2) == 1000) {
                    offsetX = (int)o.gpr3;
                    o.gpr3 += 120 * s.deltaTime * (o.gpr2 < 0 ? -1 : 1);
                    if (std::abs(o.gpr3) >= 30) {
                        o.x += o.gpr3;
                        o.gpr2 = o.velocityAngleDeg() - 90;
                        offsetX = 0;
                    }
                } else {
                    float aux = OBJ_DATA[dataId][std::abs(o.gpr3) >= 30 ? 3 : 1];
                    o.x += aux * s.deltaTime * Trig::cos(o.gpr2);
                    o.y += aux * s.deltaTime * Trig::sin(o.gpr2);
                }
                break;
            }
            case 11: {
                float aux = o.x;
                if (o.x <= SCREEN_CENTER_W) {
                    o.x = (SCREEN_CENTER_W - TILE_SIZE_X2) * -Trig::cos(o.gpr2);
                } else {
                    o.x = (SCREEN_WIDTH - TILE_SIZE_X2) + (SCREEN_CENTER_W - TILE_SIZE_X2) * -Trig::cos(o.gpr2);
                }
                float dir = o.x > aux ? RIGHT : LEFT;
                if (dir != o.gpr3) {
                    flip = dir == LEFT ? 1 : 0;
                    o.gpr3 = dir;
                }
                if (o.x < 0) o.x = SCREEN_WIDTH - TILE_SIZE_X2;
                else if (o.x > SCREEN_WIDTH - TILE_SIZE_X2) o.x = 0;
                o.gpr2 += OBJ_DATA[dataId][0] * s.deltaTime;
                o.y += OBJ_DATA[dataId][1] * s.deltaTime;
                break;
            }
            case 12: case 16:
                o.x += o.gpr2 * s.deltaTime;
                o.y += OBJ_DATA[dataId][1] * s.deltaTime;
                break;
            case 13: {
                if (o.gpr2 == 0) {
                    if (o.gpr3 > 0 && o.x - TILE_SIZE > SCREEN_CENTER_W + TILE_SIZE_X4) {
                        o.gpr2 = 315;
                        o.gpr3 = o.y + TILE_SIZE_X4 + TILE_SIZE / 2.0f;
                    } else if (o.gpr3 < 0 && o.x + TILE_SIZE < SCREEN_CENTER_W - TILE_SIZE * 6) {
                        o.gpr2 = -135;
                        o.gpr3 = o.y + TILE_SIZE_X4 + TILE_SIZE / 2.0f;
                    } else {
                        o.x += OBJ_DATA[dataId][0] * s.deltaTime * o.gpr3;
                        o.y += OBJ_DATA[dataId][1] * s.deltaTime;
                    }
                } else if (o.gpr2 == 999) {
                    o.x += OBJ_DATA[dataId][0] * s.deltaTime * o.gpr3;
                    o.y += OBJ_DATA[dataId][1] * -s.deltaTime;
                } else {
                    o.x = SCREEN_CENTER_W - TILE_SIZE + TILE_SIZE * 9 * Trig::cos(o.gpr2);
                    o.y = o.gpr3 + TILE_SIZE * 6 * Trig::sin(o.gpr2);
                    o.gpr2 += o.gpr2 < 0 ? -0.9f : 0.9f;
                    if (o.gpr2 < -405 || o.gpr2 >= 585) {
                        o.gpr3 = o.gpr2 > 0 ? 1.0f : -1.0f;
                        o.gpr2 = 999;
                    }
                }
                break;
            }
            case 20: case 21: {
                if (o.gpr3 >= 0) {
                    offsetX = (int)(-TILE_SIZE_X4 * Trig::sin(o.gpr3));
                    o.gpr3 += OBJ_DATA[dataId][0] * s.deltaTime;
                }
                o.y += OBJ_DATA[dataId][1] * s.deltaTime;
                break;
            }
            case 38: case 39: {
                GameObj* parent = o.shadowRef;
                o.x = parent->x;
                if (o.gpr3 == 0) o.y = parent->y;
                offsetX = (int)o.gpr2;
                offsetY = (int)o.gpr1;
                break;
            }
            case 50:
                continue;
            case 51: {
                if (o.y < SCREEN_CENTER_H - TILE_SIZE) {
                    o.y += 30 * s.deltaTime;
                } else if (s.player.state == Player::CUTSCENE_STILL) {
                    destroyPortal();
                    s.player.state = Player::CUTSCENE_WALK;
                } else if (s.player.state == Player::CUTSCENE_WALK) {
                    if (s.player.y > SCREEN_CENTER_H + TILE_SIZE_X2 + 3) {
                        s.player.y += -30 * s.deltaTime;
                    } else {
                        s.player.state = Player::CUTSCENE_FADE;
                    }
                }
                break;
            }
            case 60:
                continue;
            default:
                o.x += OBJ_DATA[dataId][0] * s.deltaTime;
                o.y += OBJ_DATA[dataId][1] * s.deltaTime;
                break;
        }

        o.prevX = oldX; o.prevY = oldY; o.hasPrev = true;

        if (o.y <= s.visibleBottomY() + TILE_SIZE) {
            o.hidden = false;
            if (flip >= 0) {
                o.flipX = flip == 1;
            } else {
                o.renderOffsetX = (float)offsetX;
                o.renderOffsetY = (float)offsetY;
            }
        } else if (objId == 39) {
            destroyShadow(*o.shadowRef);
        } else {
            destroyObject(o);
        }
    }
}

// ---------------------------------------------------------------- spawn / destroy

GameObj& ObjectSystem::allocateObject(int objId, float x, float y, float gpr1, float gpr2, float gpr3) {
    auto o = std::make_unique<GameObj>();
    GameObj& ref = *o;
    engine_.state.obj.push_back(std::move(o));
    ref.id = objId;
    ref.x = x; ref.y = y;
    ref.gpr1 = gpr1; ref.gpr2 = gpr2; ref.gpr3 = gpr3;
    ref.shadowRef = nullptr;
    ref.hidden = false;
    ref.hasPrev = false;
    ref.layer = 1;
    ref.hitW = -1; ref.hitH = -1; ref.aux1 = -1;
    return ref;
}

void ObjectSystem::replaceSpriteSkin(GameObj& o, int objId, float x, float y) {
    const ObjTile& t = OBJ_TILE[objId];
    o.region = engine_.assets.obj(t.x, t.y, t.w, t.h);
    o.x = x; o.y = y;
    o.flipX = false;
}

GameObj& ObjectSystem::spawnObject(int objId, float x, float y, int objCfg) {
    GameObj& o = allocateObject(objId, x, y, 0, 0, 0);
    GameState& s = engine_.state;

    int dataId = OBJ_TILE[objId].dataId;
    o.gpr1 = OBJ_DATA[dataId][2];
    o.gpr2 = OBJ_DATA[dataId][3];
    o.gpr3 = OBJ_DATA[dataId][4];

    float spawnSpeed = 0;
    float offsetX = 0, offsetY = 0;
    int layer = 1;
    bool useBossBuffer = false;

    switch (objId) {
        case 2: case 3: case 14: {
            spawnSpeed = objId == 2 ? 500.0f : 200.0f;
            if (x < SCREEN_CENTER_W) { o.gpr2 += 180; o.gpr3 = LEFT; } else { o.gpr3 = RIGHT; }
            if (objId == 14) { o.y = SCREEN_HEIGHT - TILE_SIZE_X4; spawnSpeed = 1000; }
            spawnShadow(o, 0, TILE_SIZE_X2 + TILE_SIZE / 2.0f, false, 0);
            break;
        }
        case 4:
            spawnSpeed = 500;
            break;
        case 5:
            o.gpr3 = x < SCREEN_CENTER_W ? LEFT : RIGHT;
            o.x = o.gpr3 == LEFT ? 0 : SCREEN_WIDTH - TILE_SIZE_X2;
            o.y = TILE_SIZE_X4;
            spawnShadow(o, 0, TILE_SIZE_X2, false, 0);
            break;
        case 6:
            spawnShadow(o, 0, TILE_SIZE_X2 + TILE_SIZE / 2.0f, false, 0);
            spawnSpeed = 400;
            break;
        case 7:
            if (objCfg == 1) { offsetX = TILE_SIZE_X4; o.gpr2 = 2; }
            else if (objCfg == 2) { offsetX = TILE_SIZE * 6; o.gpr2 = 3; }
            else {
                if (x < SCREEN_CENTER_W) { o.x = TILE_SIZE_X4; o.gpr2 = RIGHT; }
                else { o.x = SCREEN_WIDTH - TILE_SIZE * 6; o.gpr2 = LEFT; }
            }
            break;
        case 8: case 12: {
            float angle = std::min<float>(20, atan3(std::abs(o.x - s.player.x), std::abs(o.y - s.player.y)));
            o.gpr2 = OBJ_DATA[dataId][0] * (o.x > s.player.x ? LEFT : RIGHT) * Trig::sin(angle);
            spawnShadow(o, 0, TILE_SIZE_X2, false, 0);
            if (objId == 8) objId = 5;
            break;
        }
        case 9:
            spawnSpeed = 300;
            if (x < SCREEN_CENTER_W) { o.x = TILE_SIZE_X4; o.gpr2 = 180; o.gpr3 = LEFT; }
            else { o.x = SCREEN_WIDTH - TILE_SIZE_X4 - TILE_SIZE_X2; o.gpr2 = 1; o.gpr3 = RIGHT; }
            break;
        case 10:
            if (objCfg != 0) { offsetX = TILE_SIZE_X2; o.gpr2 = objCfg; }
            else o.gpr2 = Trig::atan2(s.player.y - y, s.player.x - x);
            break;
        case 11:
            spawnSpeed = 400;
            if (x < SCREEN_CENTER_W) { o.x = 0; o.gpr2 = 90; } else { o.x = SCREEN_WIDTH; o.gpr2 = 270; }
            spawnShadow(o, 0, TILE_SIZE_X2 + TILE_SIZE / 2.0f, false, 0);
            break;
        case 13:
            o.x = o.x < SCREEN_CENTER_W ? 0.0f : SCREEN_WIDTH - TILE_SIZE_X2;
            o.y = TILE_SIZE_X2;
            o.gpr3 = o.x == 0 ? 1.0f : -1.0f;
            spawnShadow(o, 0, TILE_SIZE_X2 + TILE_SIZE / 2.0f, false, 0);
            spawnSpeed = 300;
            break;
        case 16: {
            float angle = atan3(o.x - s.player.x, o.y - s.player.y - 200);
            o.gpr2 = OBJ_DATA[dataId][0] * Trig::sin(-angle);
            break;
        }
        case 31:
            offsetX = FIRE_ANIM[0] * TILE_SIZE_X2;
            layer = 4;
            if (objCfg != 0) engine_.assets.playSfx("ENEMY_KILL");
            break;
        case 50:
            useBossBuffer = true;
            break;
    }

    if (objCfg > 0 && spawnSpeed > 0) {
        engine_.actionQueueSystem.enqueue(objCfg, objId, x, y, 0, (long long)spawnSpeed);
    }

    if (isEnemy(objId)) s.enemiesCount++;

    o.id = objId;
    o.layer = layer;
    const ObjTile& t = OBJ_TILE[objId];
    o.region = useBossBuffer ? engine_.assets.boss(t.x + (int)offsetX, t.y + (int)offsetY, t.w, t.h)
                             : engine_.assets.obj(t.x + (int)offsetX, t.y + (int)offsetY, t.w, t.h);
    o.flipX = false;
    o.y = std::max(-7.0f, o.y);
    return o;
}

float ObjectSystem::atan3(float a, float b) {
    // Port of MMBasic's MATH(ATAN3 y,x) extension: returns an unsigned angle in [0,360).
    float v = Trig::atan2(a, b);
    return v < 0 ? v + 360 : v;
}

void ObjectSystem::spawnShadow(GameObj& obj, float offsetX, float offsetY, bool fixedY, int shadowTileId) {
    auto shadowPtr = std::make_unique<GameObj>();
    GameObj& shadow = *shadowPtr;
    engine_.state.obj.push_back(std::move(shadowPtr));

    int id = shadowTileId != 0 ? shadowTileId : 39;
    shadow.id = 39;
    shadow.x = obj.x + offsetX;
    shadow.y = obj.y + offsetY;
    shadow.gpr1 = offsetY;
    shadow.gpr2 = offsetX;
    shadow.gpr3 = fixedY ? 1.0f : 0.0f;
    shadow.shadowRef = &obj;
    shadow.layer = 3;
    shadow.hidden = false;
    obj.shadowRef = &shadow;
    const ObjTile& t = OBJ_TILE[id];
    shadow.region = engine_.assets.obj(t.x, t.y, t.w, t.h);
}

void ObjectSystem::spawnPrincess() {
    spawnObject(51, SCREEN_CENTER_W - OBJ_TILE[51].w / 2.0f, 0, 0);
}

void ObjectSystem::createPortal() {
    GameState& s = engine_.state;
    GameObj& o = allocateObject(60, SCREEN_WIDTH / 2.0f - TILE_SIZE_X2, TILE_SIZE, 0, 0, 0);
    o.layer = 1;
    o.hidden = true; // invisible collision volume, matches the original's "read background pixels" trick
    o.hitW = TILE_SIZE_X4;
    o.hitH = TILE_SIZE * 4 - 2;
    s.portalObj = &o;
}

void ObjectSystem::destroyPortal() {
    GameState& s = engine_.state;
    if (s.portalObj == nullptr) return;
    destroyObject(*s.portalObj);
    s.portalObj = nullptr;
}

void ObjectSystem::spawnBridge(float x, float y, bool /*down*/) {
    // Purely visual: a static bridge decoration object drawn over the gap tiles.
    GameObj& o = allocateObject(48, x - TILE_SIZE, y, 0, 0, 0);
    o.layer = 1;
    const ObjTile& t = OBJ_TILE[48];
    o.region = engine_.assets.obj(t.x, t.y, t.w, t.h);
}

void ObjectSystem::destroyObject(GameObj& o) {
    if (o.id <= 0) return;
    int objId = o.id;
    destroyShadow(o);
    if (isEnemy(objId)) {
        GameState& s = engine_.state;
        s.enemiesCount--;
        if (s.boss.status > 1) engine_.bossSystem.bossEnemyDestroyed(objId, o.gpr2, o.gpr3);
    }
    o.free();
}

void ObjectSystem::destroyShadow(GameObj& source) {
    GameObj* shadow = source.shadowRef;
    if (shadow == nullptr) return;
    source.shadowRef = nullptr;
    shadow->free();
}

void ObjectSystem::destroyAll() {
    GameState& s = engine_.state;
    for (auto& oPtr : s.obj) if (oPtr->id > 0) destroyObject(*oPtr);
    for (auto& shPtr : s.playerShots) if (!shPtr->isFree()) destroyShot(*shPtr);
    for (auto& shPtr : s.enemyShots) if (!shPtr->isFree()) destroyShot(*shPtr);
    for (auto& aPtr : s.queue) aPtr->execCount = 0;
}

void ObjectSystem::destroyBlock(Block& b) {
    b.free();
}

void ObjectSystem::killAllEnemies(bool sfx) {
    GameState& s = engine_.state;
    if (sfx) engine_.assets.playSfx("KILL_ALL_ENEMIES");
    size_t n = s.obj.size();
    for (size_t i = 0; i < n; i++) {
        GameObj& o = *s.obj[i];
        if (o.id == 0 || o.id > 50) continue;
        engine_.collisionSystem.hitObject(o, false, true);
    }
}

void ObjectSystem::killPlayer() {
    GameState& s = engine_.state;
    s.scrollOn = false;
    s.contSfx = "";
    engine_.assets.playSong("SILENCE");
    engine_.assets.playSfx("PLAYER_DEATH");
    engine_.initPlayer(s.player.lives - 1);
    s.player.state = Player::DEAD;
    checkShield();
}

void ObjectSystem::killEnemy(GameObj& o, bool keepObject) {
    if (o.id <= (int)OBJ_POINTS.size() - 1) engine_.powerUpSystem.incrementScore(OBJ_POINTS[o.id]);
    startFireAnimation(o.x, o.y, true);
    engine_.assets.playSfx("ENEMY_KILL");
    if (!keepObject) destroyObject(o);
}

void ObjectSystem::startFireAnimation(float x, float y, bool sfx) {
    engine_.actionQueueSystem.enqueue(1, 34, x, y, sfx ? 1.0f : 0.0f, 0);
}

void ObjectSystem::destroyShot(Shot& sh, bool animate) {
    GameState& s = engine_.state;
    float x = sh.x, y = sh.y;
    // The boomerang has no per-shot sfx (fire() plays it as a looping "contSfx" instead, for as
    // long as the shot is in flight - see fire()); every other place that stops a shot early
    // (hitBlock/hitObject) already clears it, but a shot that simply expires (flies off screen,
    // case 33's spin timeout) went through here uncleared and the sound looped forever.
    if (sh.weaponId == 4 || sh.weaponId == 9) s.contSfx = "";
    sh.free();
    if (animate) startFireAnimation(x, y, true);
}

void ObjectSystem::destroyEnemiesShots() {
    for (auto& shPtr : engine_.state.enemyShots) if (!shPtr->isFree()) destroyShot(*shPtr);
}

// ---------------------------------------------------------------- shield

void ObjectSystem::spawnShield() {
    GameState& s = engine_.state;
    s.player.powerUp = Player::PWR_SHIELD;
    s.player.shield = SHIELD_MAX_HITS;
    s.player.shieldSpriteVisible = true;
}

void ObjectSystem::checkShield() {
    Player& p = engine_.state.player;
    if (p.shield <= 0) {
        p.powerUp = Player::PWR_NONE;
        p.shield = 0;
        p.shieldSpriteVisible = false;
    }
}

// ---------------------------------------------------------------- weapon fire

void ObjectSystem::fire() {
    GameState& s = engine_.state;
    Player& p = s.player;
    if (s.clockMs - p.lastShotTimeMs < weaponCooldownMs(p.weapon) || p.powerUp == Player::PWR_INVINCIBLE) return;
    // The boomerang isn't paced by a cooldown like other weapons: up to maxBoomerangs can be in
    // flight at once (2 normally, 3 with the speed-boost weapon variant), and throwing another is
    // only blocked while that many are still out - freeing up again the instant one of them
    // returns and is caught by the player (see CollisionSystem's "player vs its own returning
    // shots" check), exactly like the original's per-slot behaviour.
    if (p.weapon == 4 || p.weapon == 9) {
        int maxBoomerangs = p.weapon == 9 ? 3 : 2;
        long inFlight = 0;
        for (auto& shPtr : s.playerShots) if (shPtr->weaponId == 4 || shPtr->weaponId == 9) inFlight++;
        if (inFlight >= maxBoomerangs) return;
    }

    int shots = p.weapon > 6 ? 2 : 1;
    int tileId = 40;
    float speedX = 0, speedY = -220;
    std::string sfx = "SHOT";
    float x = p.x + (TILE_SIZE - OBJ_TILE[tileId].w / 2.0f);
    float y = p.y - OBJ_TILE[tileId].h - 1;

    s.fire = true;
    switch (p.weapon) {
        case 2: tileId = 41; break;
        case 3: case 8:
            shots = 2; x -= TILE_SIZE_X2; speedY = -200; tileId = 42; sfx = "FLAME";
            break;
        case 4: case 9: tileId = 43; sfx = ""; s.contSfx = "BOOMERANG"; break;
        case 5: case 10: shots = 2; tileId = p.weapon == 5 ? 45 : 46; sfx = "SWORD"; break;
        case 6: case 11: tileId = 47; sfx = "FIRE_ARROW"; break;
    }

    // The player shot pool is unbounded (see GameState): pacing a volley used to also depend on
    // whether the previous shot(s) still occupied a fixed slot, now it's purely the per-weapon
    // cooldown checked above.
    for (int i = 0; i <= shots; i++) {
        float offset = 0;

        if (p.weapon == 3 || p.weapon == 8) {
            if (i > 0) x += TILE_SIZE_X2;
            if (i == 1) offset = TILE_SIZE;
            if (p.weapon == 3) speedX = 50.0f * i - 50.0f;
        }

        auto sh = makeShot();
        sh->weaponId = p.weapon;
        sh->x = x;
        sh->y = y - offset;
        sh->gpr1 = speedX;
        sh->gpr2 = speedY;
        const ObjTile& t = OBJ_TILE[tileId];
        sh->region = engine_.assets.obj(t.x, t.y, t.w, t.h);
        sh->rot = 0;
        s.playerShots.push_back(std::move(sh));

        engine_.assets.playSfx(sfx);
        p.lastShotTimeMs = s.clockMs;
        if (p.weapon != 3 && p.weapon != 8) break;
    }
}

void ObjectSystem::enemiesFire() {
    GameState& s = engine_.state;
    if (s.freezeTimer >= 0) return;
    size_t n = s.obj.size();
    for (size_t i = 0; i < n; i++) {
        GameObj& o = *s.obj[i];
        int id = o.id;
        if (id == 0 || (s.stage < 5 && o.y < TILE_SIZE_X2)) continue;
        if (id < 20 && id != 8) enemyFire(o, false);
    }
}

void ObjectSystem::enemyFire(GameObj& o, bool disableRnd) {
    GameState& s = engine_.state;
    int objId = o.id;
    if (!disableRnd && Rnd::value() > SHOOT_CHANCE[s.difficulty][s.stage - 1]) return;
    if (s.stage < 6 && o.y > MAX_ENEMIES_SHOOT_Y) return;

    Shot* shot = &getFreeShotSlot();

    float x = o.x + OBJ_TILE[objId].w / 2.0f;
    float y = o.y + OBJ_TILE[objId].h / 2.0f;
    float angle = o.velocityAngleDeg();
    int rot = 0, weaponIx = 23;
    int calcId = objId;
    float gpr1 = 0, gpr2 = 0, gpr3 = 0;
    int offsetX = 0;
    float speed = 70;

    if (calcId == 50) calcId += s.stage;

    switch (calcId) {
        case 4: case 57: {
            if (calcId == 57) speed = 100;
            if (angle >= 23 && angle < 67) { weaponIx = 31; rot = 3; x += TILE_SIZE; y += -TILE_SIZE * 2.5f; }
            else if (angle >= 292 && angle < 337) { weaponIx = 31; rot = 2; x += -TILE_SIZE_X2; y += -TILE_SIZE * 2.5f; }
            else if (angle >= 112 && angle < 157) { weaponIx = 31; rot = 1; x += TILE_SIZE; y += TILE_SIZE; }
            else if (angle >= 202 && angle < 247) { weaponIx = 31; x += -TILE_SIZE_X2; y += TILE_SIZE; }
            else if (angle >= 67 && angle < 112) { weaponIx = 30; rot = 1; x += TILE_SIZE; }
            else if (angle >= 247 && angle < 292) { weaponIx = 30; x += -TILE_SIZE * 2.5f; }
            else if (angle >= 157 && angle < 202) { weaponIx = 29; y += TILE_SIZE; }
            else { weaponIx = 29; rot = 2; y += -TILE_SIZE * 2.5f; }
            break;
        }
        case 5:
            return;
        case 7:
            if (o.gpr3 != 0 || o.gpr2 > 1) return;
            weaponIx = 25;
            break;
        case 8: {
            weaponIx = 24; speed *= 1.3f;
            createShot(getFreeShotSlot(), weaponIx, x, y, speed * Trig::sin(angle - 30), speed * -Trig::cos(angle - 30), 0, 0, 0);
            createShot(getFreeShotSlot(), weaponIx, x, y, speed * Trig::sin(angle - 10), speed * -Trig::cos(angle - 10), 0, 0, 0);
            createShot(getFreeShotSlot(), weaponIx, x, y, speed * Trig::sin(angle + 10), speed * -Trig::cos(angle + 10), 0, 0, 0);
            createShot(getFreeShotSlot(), weaponIx, x, y, speed * Trig::sin(angle + 30), speed * -Trig::cos(angle + 30), 0, 0, 0);
            return;
        }
        case 9: case 16:
            weaponIx = 24; speed = 100;
            break;
        case 12: case 13:
            if (s.stage > 2 && s.row > MAP_ROWS / 2) return;
            break;
        case 15:
            for (int i = 45; i <= 315; i += 45) {
                if (i != 180) createShot(*shot, weaponIx, x, y, speed * -Trig::cos((float)i), speed * Trig::sin((float)i), 0, 0, 0);
                if (i < 315) shot = &getFreeShotSlot();
            }
            return;
        case 51:
            weaponIx = 24; speed = 120;
            break;
        case 52:
            weaponIx = 33;
            gpr1 = o.x;
            gpr2 = o.y + TILE_SIZE * 8;
            break;
        case 53: {
            weaponIx = 28;
            angle = std::min(200.0f, std::max(160.0f, angle));
            speed = 110;
            for (int i = -60; i <= 60; i += 20) {
                createShot(*shot, weaponIx, x, y, speed * Trig::sin(angle - i), speed * -Trig::cos(angle - i), 0, 0, 0);
                if (i < 60) shot = &getFreeShotSlot();
            }
            return;
        }
        case 54: case 56: case 58: {
            weaponIx = 28; speed = 110;
            angle = std::max(std::min(210.0f, angle), 150.0f);
            gpr1 = speed * Trig::sin(angle);
            gpr2 = speed * -Trig::cos(angle);
            if (s.stage == 4) { offsetX = TILE_SIZE_X2; gpr3 = s.player.y; }
            break;
        }
        case 55: {
            weaponIx = 32; gpr2 = 100;
            x = o.x - TILE_SIZE;
            createShot(getFreeShotSlot(), weaponIx, x, y, gpr1, gpr2, gpr3, rot, offsetX);
            x += TILE_SIZE * 6;
            shot = &getFreeShotSlot();
            break;
        }
    }

    if (gpr1 == 0 && gpr2 == 0) {
        gpr1 = speed * Trig::sin(angle);
        gpr2 = speed * -Trig::cos(angle);
    }
    createShot(*shot, weaponIx, x, y, gpr1, gpr2, gpr3, rot, offsetX);
}

void ObjectSystem::createShot(Shot& shot, int weaponIx, float x, float y, float gpr1, float gpr2, float gpr3, int rot, int offsetX) {
    shot.weaponId = weaponIx;
    shot.x = x; shot.y = y;
    shot.gpr1 = gpr1; shot.gpr2 = gpr2; shot.gpr3 = gpr3;
    shot.rot = rot;
    shot.offsetX = offsetX;
    const ObjTile& t = OBJ_TILE[weaponIx];
    shot.region = engine_.assets.obj(t.x + offsetX, t.y, t.w, t.h);
}
