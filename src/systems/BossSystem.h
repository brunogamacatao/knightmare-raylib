#pragma once

struct GameObj;

// Port of boss.inc: spawning, AI movement, hit boxes and death sequence for all eight stage bosses.
class BossSystem {
public:
    explicit BossSystem(class Engine& engine) : engine_(engine) {}

    void activateBoss();
    void bossEnemyDestroyed(int objId, float gpr1, float gpr2);
    // Returns true if the hit destroys the projectile that caused it.
    bool hitBoss(GameObj& hit);
    void killBoss();
    void destroyBoss();
    void spawnBoss();
    void createBossHitBox(int ix, float offsetX, float offsetY, float width, float life);
    void createBossShield(float offsetX, float offsetY, float width);
    void showBossBody(GameObj* body, int tileOffset);
    void createBossEye(int eyeId);
    void bossFire();
    void animateBoss();
    void scrollBoss();
    void moveBoss();
    void activateHitBox(int i, bool active);
    void activateShield(bool active);
    bool isHitBoxActive(int i) const;
    void moveHitBox();

private:
    Engine& engine_;
};
