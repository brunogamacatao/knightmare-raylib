#pragma once

struct GameObj;
struct Shot;
struct Block;

// Port of objects.inc: player/enemy/object animation, movement, spawning and weapon fire.
class ObjectSystem {
public:
    explicit ObjectSystem(class Engine& engine) : engine_(engine) {}

    // ---------------------------------------------------------------- slots
    Shot& getFreeShotSlot();
    static bool isEnemy(int objId) { return objId > 0 && objId < 20; }

    // ---------------------------------------------------------------- input / player movement
    void processInput();
    void movePlayer(int direction);
    void autoMovePlayerToPortal();
    void movePlayerTo(float x, float y);
    void movePlayerToPortal();

    // ---------------------------------------------------------------- animation
    void animatePlayer();
    void animateObjects(bool reload);
    void animateShots();
    void fadeMap(int start);

    // ---------------------------------------------------------------- movement
    void moveShots();
    void moveAndProcessObjects();

    // ---------------------------------------------------------------- spawn / destroy
    GameObj& allocateObject(int objId, float x, float y, float gpr1, float gpr2, float gpr3);
    void replaceSpriteSkin(GameObj& o, int objId, float x, float y);
    GameObj& spawnObject(int objId, float x, float y, int objCfg);
    void spawnShadow(GameObj& obj, float offsetX, float offsetY, bool fixedY, int shadowTileId);
    void spawnPrincess();
    void createPortal();
    void destroyPortal();
    void spawnBridge(float x, float y, bool down);
    void destroyObject(GameObj& o);
    void destroyShadow(GameObj& source);
    void destroyAll();
    void destroyBlock(Block& b);
    void killAllEnemies(bool sfx);
    void killPlayer();
    void killEnemy(GameObj& o, bool keepObject);
    void startFireAnimation(float x, float y, bool sfx);
    void destroyShot(Shot& sh, bool animate = false);
    void destroyEnemiesShots();

    // ---------------------------------------------------------------- shield
    void spawnShield();
    void checkShield();

    // ---------------------------------------------------------------- weapon fire
    void fire();
    void enemiesFire();
    void enemyFire(GameObj& o, bool disableRnd);
    void createShot(Shot& shot, int weaponIx, float x, float y, float gpr1, float gpr2, float gpr3, int rot, int offsetX);

private:
    static constexpr int KB_LEFT = 0, KB_RIGHT = 1, KB_UP = 2, KB_DOWN = 3;

    Engine& engine_;

    void applyFrame(GameObj& o, int objId, int effect, int offsetX, int offsetY, int offsetH);
    void animateShot(Shot& sh);
    void setShotRegion(Shot& sh);
    void moveShot(Shot& sh);
    static float atan3(float a, float b);
};
