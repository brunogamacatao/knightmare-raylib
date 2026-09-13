#pragma once

#include <array>

struct GameObj;
struct Block;
struct Shot;

// Port of collision.inc. The original relies on the MMBasic hardware sprite engine's bounding-box
// collision interrupt; here the same pairings are evaluated directly with AABB checks every tick.
class CollisionSystem {
public:
    explicit CollisionSystem(class Engine& engine) : engine_(engine) {}

    void update();

    // Returns true if the hit destroys the block's projectile.
    bool hitBlock(Block& b);
    // Returns true if the hit destroys the shot.
    bool hitShot(const Shot& other) const;
    // Returns true if the hit destroys the projectile that caused it.
    bool hitObject(GameObj& o, bool sfx, bool instantKill);
    // Returns true if the hit destroys the shot.
    bool hitShield(Shot& sh);
    // Returns true if the hit kills the player. obj may be nullptr when caused by an enemy shot.
    bool hitPlayer(GameObj* obj, bool shotHit);

private:
    Engine& engine_;
    using Box = std::array<float, 4>; // x, y, w, h

    bool isBossBody(const GameObj& o) const;
    Box playerBox() const;
    Box shieldBox() const;
    Box shotBox(const Shot& sh) const;
    Box blockBox(const Block& b) const;
    Box objBox(const GameObj& o) const;
    static bool overlap(const Box& a, const Box& b);
    bool circleOverlap(const GameObj& o) const;
    static float radius(float width, float height);
    void playerHitObj(GameObj& o);
};
