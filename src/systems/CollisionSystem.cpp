#include "CollisionSystem.h"

#include <cmath>
#include "../Engine.h"
#include "../Trig.h"

using namespace Constants;

void CollisionSystem::update() {
    GameState& s = engine_.state;
    Player& p = s.player;

    // Player shots vs blocks, objects and enemy shots.
    size_t nPlayerShots = s.playerShots.size();
    for (size_t i = 0; i < nPlayerShots; i++) {
        Shot& sh = *s.playerShots[i];
        if (sh.isFree()) continue;
        Box sb = shotBox(sh);

        bool destroyed = false;
        size_t nBlocks = s.blocks.size();
        for (size_t j = 0; j < nBlocks; j++) {
            Block& b = *s.blocks[j];
            if (b.isFree() || b.hidden) continue;
            if (overlap(sb, blockBox(b))) { if (hitBlock(b)) { destroyed = true; break; } }
        }
        if (destroyed) { engine_.objectSystem.destroyShot(sh); continue; }

        size_t nObj = s.obj.size();
        for (size_t j = 0; j < nObj; j++) {
            GameObj& o = *s.obj[j];
            if (o.id == 0 || o.hidden || isBossBody(o)) continue;
            if (overlap(sb, objBox(o))) { if (hitObject(o, true, false)) { destroyed = true; break; } }
        }
        if (destroyed) { engine_.objectSystem.destroyShot(sh); continue; }

        size_t nEnemyShots = s.enemyShots.size();
        for (size_t j = 0; j < nEnemyShots; j++) {
            Shot& other = *s.enemyShots[j];
            if (other.isFree()) continue;
            if (overlap(sb, shotBox(other))) {
                if (hitShot(other)) { engine_.objectSystem.destroyShot(other, true); destroyed = true; break; }
            }
        }
        if (destroyed) engine_.objectSystem.destroyShot(sh);
    }

    // Enemy shots vs player and shield.
    size_t nEnemyShots2 = s.enemyShots.size();
    for (size_t i = 0; i < nEnemyShots2; i++) {
        Shot& sh = *s.enemyShots[i];
        if (sh.isFree()) continue;
        Box sb = shotBox(sh);

        if (p.shield > 0 && p.shieldSpriteVisible && overlap(sb, shieldBox())) {
            if (hitShield(sh)) { engine_.objectSystem.destroyShot(sh); continue; }
        }
        if (overlap(sb, playerBox())) {
            if (hitPlayer(nullptr, true)) engine_.objectSystem.destroyShot(sh);
        }
    }

    // Player vs blocks.
    size_t nBlocks2 = s.blocks.size();
    for (size_t i = 0; i < nBlocks2; i++) {
        Block& b = *s.blocks[i];
        if (b.isFree() || b.hidden) continue;
        if (overlap(playerBox(), blockBox(b))) engine_.mapSystem.collectBlockBonus(b);
    }

    // Player vs its own returning shots (only relevant for the boomerang, which the player catches
    // on the way back down). A boomerang spawns right above the player (same as every other
    // weapon's shot), so its box already overlaps the player's for the first instant of flight -
    // gating this on gpr2 > 0 (moveShot()'s vertical speed for weapon 4/9, negative while still
    // rising) makes it only catchable once it's actually falling back towards the player, instead
    // of being caught the instant it spawns and never visibly leaving.
    if (p.weapon == 4 || p.weapon == 9) {
        size_t nPS = s.playerShots.size();
        for (size_t i = 0; i < nPS; i++) {
            Shot& sh = *s.playerShots[i];
            if (sh.isFree()) continue;
            if (sh.gpr2 > 0 && overlap(playerBox(), shotBox(sh))) {
                engine_.objectSystem.destroyShot(sh);
            }
        }
    }

    // Player vs objects (crystals, contact damage, portal).
    size_t nObj2 = s.obj.size();
    for (size_t i = 0; i < nObj2; i++) {
        GameObj& o = *s.obj[i];
        if (o.id == 0 || o.hidden || isBossBody(o)) continue;
        // Pickups (weapon crystal / power-up) are drawn as small round sprites; an AABB test
        // against their tile box missed them at the corners far more often than it should have, so
        // these two use a circle-vs-circle distance test instead.
        bool hit = (o.id == 20 || o.id == 21) ? circleOverlap(o) : overlap(playerBox(), objBox(o));
        if (hit) playerHitObj(o);
    }
}

bool CollisionSystem::isBossBody(const GameObj& o) const {
    return engine_.state.boss.status > 0 && &o == engine_.state.boss.body;
}

// ---------------------------------------------------------------- bounding boxes

CollisionSystem::Box CollisionSystem::playerBox() const {
    const Player& p = engine_.state.player;
    return {p.x, p.y, (float)TILE_SIZE_X2, (float)TILE_SIZE_X2};
}

CollisionSystem::Box CollisionSystem::shieldBox() const {
    const ObjTile& t = OBJ_TILE[36];
    const Player& p = engine_.state.player;
    return {p.x, p.y - TILE_SIZE, (float)t.w, (float)t.h};
}

CollisionSystem::Box CollisionSystem::shotBox(const Shot& sh) const {
    const ObjTile& t = OBJ_TILE[sh.weaponId];
    return {sh.x, sh.y, (float)t.w, (float)t.h};
}

CollisionSystem::Box CollisionSystem::blockBox(const Block& b) const {
    return {b.x, b.y, (float)TILE_SIZE_X2, (float)TILE_SIZE_X2};
}

CollisionSystem::Box CollisionSystem::objBox(const GameObj& o) const {
    if (o.hitW > 0) return {o.x, o.y, o.hitW, o.hitH};
    const ObjTile& t = OBJ_TILE[o.id];
    return {o.x, o.y, (float)t.w, (float)t.h};
}

bool CollisionSystem::overlap(const Box& a, const Box& b) {
    return a[0] < b[0] + b[2] && a[0] + a[2] > b[0] && a[1] < b[1] + b[3] && a[1] + a[3] > b[1];
}

// distance(player center, pickup center) < player.radius + powerup.radius, with radius =
// sqrt(width * height) / 2 (for the square 16x16 sprites used here that's exactly half the side,
// the correct inscribed-circle radius of the tile).
//
// Uses the pickup's actually-rendered position (o.x/o.y plus its renderOffsetX/Y), not just
// o.x/o.y: ids 20/21 swing side to side by up to 4 tiles as a visual flourish (see
// ObjectSystem::moveAndProcessObjects case 20/21), and the hitbox must track what's on screen or
// the player ends up touching empty space where the sprite visually isn't anymore.
bool CollisionSystem::circleOverlap(const GameObj& o) const {
    Box pb = playerBox();
    const ObjTile& t = OBJ_TILE[o.id];
    float pcx = pb[0] + pb[2] / 2.0f, pcy = pb[1] + pb[3] / 2.0f;
    float ocx = o.x + o.renderOffsetX + t.w / 2.0f, ocy = o.y + o.renderOffsetY + t.h / 2.0f;
    float playerRadius = radius(pb[2], pb[3]);
    float powerupRadius = radius((float)t.w, (float)t.h);
    float dx = pcx - ocx, dy = pcy - ocy;
    float distance = std::sqrt(dx * dx + dy * dy);
    return distance < playerRadius + powerupRadius;
}

float CollisionSystem::radius(float width, float height) {
    return std::sqrt(width * height) / 2.0f;
}

// ---------------------------------------------------------------- reactions

void CollisionSystem::playerHitObj(GameObj& o) {
    GameState& s = engine_.state;
    if (s.player.state == Player::MOVING_TO_PORTAL) return;
    switch (o.id) {
        case 20:
            engine_.powerUpSystem.changeWeapon(o);
            engine_.objectSystem.destroyObject(o);
            break;
        case 21:
            engine_.powerUpSystem.powerUp(o);
            engine_.objectSystem.destroyObject(o);
            break;
        case 60:
            // Portal / boss hit boxes: intangible to the player, they only react to weapon fire.
            break;
        default:
            if (s.player.powerUp == Player::PWR_INVINCIBLE) {
                hitObject(o, true, true);
            } else {
                hitPlayer(&o, false);
            }
            break;
    }
}

bool CollisionSystem::hitBlock(Block& b) {
    int maxHits = engine_.mapSystem.blockMaxHits(b);
    // Once fully cracked open (hits reached maxHits) shots already passed through below - but
    // blockMaxHits() returns 0 once the block is actually collected (b.type==COLLECTED), so b.hits
    // (already >= the original maxHits) can never equal that new 0 again: without this explicit
    // COLLECTED check, a collected block would silently keep absorbing/blocking shots forever
    // instead of letting them through.
    if (b.type == Block::COLLECTED || b.hits >= maxHits) return false;

    b.hits++;
    engine_.state.contSfx = "";
    if (b.hits < maxHits) engine_.assets.playSfx("BLOCK_HIT");
    else if (b.hits == maxHits) engine_.assets.playSfx("BLOCK_OPEN");

    if (b.hits == 1 || b.hits == maxHits) {
        engine_.actionQueueSystem.enqueueBlock(1, 22, &b, 1);
    }
    return true;
}

bool CollisionSystem::hitShot(const Shot& other) const {
    return other.weaponId == 25 || other.weaponId == 33;
}

// Returns true if the hit destroys the projectile that caused it. Mirrors hit_object()'s somewhat
// unusual control flow: every case falls through to a trailing "non-super weapons always destroy
// the shot" rule, except the two paths that have an explicit early exit in the original.
bool CollisionSystem::hitObject(GameObj& o, bool sfx, bool instantKill) {
    GameState& s = engine_.state;
    int objId = o.id;
    bool isDead = true;
    bool result = false;
    s.contSfx = "";

    if (objId == 5) {
        if (instantKill) {
            engine_.objectSystem.startFireAnimation(o.x, o.y, true);
            engine_.objectSystem.destroyObject(o);
        } else {
            float angle = o.velocityAngleDeg();
            o.gpr1 = 1;
            o.gpr2 = CLOUD_ATTACK_SPEED * Trig::sin(angle) + 0.01f;
            o.gpr3 = CLOUD_ATTACK_SPEED * -Trig::cos(angle);
        }
    } else if (objId < 20) {
        switch (objId) {
            case 7:
                if (o.gpr3 > 0 || o.gpr2 > 1) return false; // already split/falling: ignore further hits
                if (o.gpr1 > 1) {
                    o.gpr3 = 1;
                    engine_.assets.playSfx("SPLIT");
                    engine_.actionQueueSystem.enqueue(1, 7, o.x, o.y, 1, 0);
                    engine_.actionQueueSystem.enqueue(1, 7, o.x, o.y, 2, 0);
                    o.hidden = true;
                }
                break;
            case 10:
                if (std::abs(o.gpr2) == 1000) return false; // already split halves: ignore further hits
                if (o.gpr3 == 0) {
                    isDead = false;
                    engine_.assets.playSfx("SPLIT");
                    engine_.actionQueueSystem.enqueue(1, 10, o.x, o.y, -1000, 0);
                    engine_.actionQueueSystem.enqueue(1, 10, o.x, o.y, 1000, 0);
                }
                break;
        }

        o.gpr1--;
        if (!instantKill && o.gpr1 > 0) {
            return engine_.powerUpSystem.isSuperWeapon() ? false : true;
        }
        if (isDead) engine_.objectSystem.killEnemy(o, false); else engine_.objectSystem.destroyObject(o);
    } else if (objId == 20 || objId == 21) {
        o.gpr2++;
        o.gpr2 = std::fmod(o.gpr2, (float)(objId == 20 ? 8 : 7));
        if (o.gpr2 > 3 && o.gpr3 < 0) o.gpr3 = 0;
        if (sfx) engine_.assets.playSfx("POWER_UP_HIT");
        result = true;
    } else if (objId == 34 || objId == 49) {
        if (instantKill && objId == 49) engine_.objectSystem.destroyObject(o);
        return false; // fire/terrain never destroys the shot, regardless of weapon
    } else if (objId >= 50) {
        if (o.gpr1 > 0) result = engine_.bossSystem.hitBoss(o);
    }

    if (!engine_.powerUpSystem.isSuperWeapon()) result = true;
    return result;
}

bool CollisionSystem::hitShield(Shot& sh) {
    GameState& s = engine_.state;
    if (s.player.shield == 0) return false;
    int force = 1;
    switch (sh.weaponId) {
        case 15: case 32: case 33:
            force = 3;
            break;
        case 28:
            force = 3;
            if (sh.gpr1 == 0 && sh.gpr2 == 0) return false;
            break;
    }
    s.player.shield -= force;
    engine_.assets.playSfx("SHIELD");
    engine_.objectSystem.checkShield();
    return true;
}

bool CollisionSystem::hitPlayer(GameObj* obj, bool shotHit) {
    GameState& s = engine_.state;
    Player& p = s.player;
    if (s.invincible || p.state > 3 || p.powerUp == Player::PWR_INVISIBLE || p.powerUp == Player::PWR_INVINCIBLE) return false;
    if (!shotHit && obj != nullptr) {
        switch (obj->id) {
            case 20: case 21: case 34:
                return false;
        }
    }
    engine_.objectSystem.killPlayer();
    return true;
}
