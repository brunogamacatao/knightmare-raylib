#pragma once

#include <deque>
#include <memory>
#include <string>
#include "Constants.h"
#include "MapData.h"
#include "QueuedAction.h"
#include "entity/Block.h"
#include "entity/BossState.h"
#include "entity/GameObj.h"
#include "entity/Player.h"
#include "entity/Shot.h"

// Holds every mutable "g_*" global from the original MMBasic source (global.inc).
//
// Entity pools are unbounded: entities are appended on spawn and swept out once dead (marked via
// isFree()/free()) at the end of every tick (see Engine::tick()). This replaces the original
// MMBasic port's fixed-size sprite slots (which mirrored the MSX's hardware sprite-count limit) -
// there is no hardware reason to keep that cap on desktop/Android/iOS.
//
// Stored as vector<unique_ptr<T>> rather than vector<T> so that raw cross-references held
// elsewhere (BossState::body/hitBox, GameObj::shadowRef, QueuedAction::blockRef, GameState::
// portalObj) stay valid pointers to a fixed heap address for the entity's whole lifetime,
// regardless of how the owning vector itself is reallocated or compacted. Systems are careful to
// null out any such reference at the same point the referenced entity is freed (mirroring what the
// original Java port already did for its GC-tracked references), so a sweep never leaves a
// dangling pointer behind - see Engine::sweepDeadEntities().
struct GameState {
    bool soundOn = true;
    bool invincible = false; // dev flag
    bool scrollOn = true;

    std::unique_ptr<MapData> map;
    int row = 0;      // current top map row (0 = bottom-most row)
    int tilePx = 0;   // sub-tile scroll pixel offset (0..7)
    // How many extra trailing map rows below the classic 22-row window a tall portrait screen has
    // room to show (set by GameRenderer on resize, from the screen's aspect ratio). Purely a
    // rendering concern: those rows are already-passed terrain the player can no longer reach, kept
    // on screen only as backdrop below the classic window, which stays the actual playfield -
    // gameplay (enemy spawns, the player's movement bounds) never looks at this.
    int extraRows = 0;
    // How many of the extra trailing rows below the classic window are currently drawn (0 at stage
    // start, growing by one row per natural scroll step up to extraRows - see MapSystem::scrollMap()
    // and GameRenderer::drawMap()).
    int revealedExtraRows = 0;
    float deltaTime = Constants::GAME_TICK_MS / 1000.0f;
    long long clockMs = 0; // free running clock, equivalent to MMBasic's `timer`
    int tick = 0;          // g_timer: incremented once per fixed game tick
    int stage = 1;
    int difficulty = 1; // 0 easy, 1 normal, 2 hard
    int enemiesCount = 0;

    long long pShotTimerMs = 0;
    float freezeTimer = -1;
    float powerUpTimer = -1;
    long long prevFrameTimerMs = 0;

    bool fire = false;
    int animTick = 0;

    int score = 0;
    int hiscore = 0;

    Player player;
    BossState boss;

    // std::deque, not std::vector: processing one entity can synchronously spawn another into the
    // very list currently being iterated (e.g. a boss hit spawning a new eye while CollisionSystem
    // is mid-iteration over `obj`). deque's push_back never invalidates references/pointers to
    // existing elements (only iterators, which systems avoid holding across such calls - they use
    // an index bounded by the size captured at the start of the loop instead). That index bound also
    // reproduces the original Java port's CopyOnWriteArrayList iteration semantics: anything spawned
    // mid-pass is simply not visited until the next tick, rather than crashing or corrupting state.
    std::deque<std::unique_ptr<Shot>> playerShots;
    std::deque<std::unique_ptr<Shot>> enemyShots;
    std::deque<std::unique_ptr<GameObj>> obj;
    std::deque<std::unique_ptr<Block>> blocks;
    std::deque<std::unique_ptr<QueuedAction>> queue;

    std::string contSfx;
    // Direct reference to the portal's invisible collision-volume object, or nullptr when none exists.
    GameObj* portalObj = nullptr;

    // fade_map() overlay state, consumed by the renderer.
    bool fadeActive = false;
    float fadeX = 0, fadeY = 0, fadeW = 0, fadeH = 0;
    bool fadeShowText = false;

    // Boss-death screen blink, consumed by the renderer (queue action 100).
    bool blinkActive = false;
    bool blinkGray = false;

    std::string centerMessage;

    // Bottom edge (in classic buffer space) of the map currently drawn on screen: the classic
    // height plus however many of the cosmetic extra trailing rows have been revealed so far (see
    // revealedExtraRows). Anything that despawns/hides objects, shots or blocks once they scroll
    // past the bottom of the screen must compare against this, not the bare classic SCREEN_HEIGHT -
    // otherwise they vanish (or, for blocks, lose their resolved sprite and show the unrevealed map
    // tile again) while still visible on a tall portrait screen.
    float visibleBottomY() const {
        return Constants::SCREEN_HEIGHT + revealedExtraRows * (float)Constants::TILE_SIZE;
    }

    void resetGlobals() {
        clockMs = 0;
        tick = 0;
        pShotTimerMs = 0;
        // clockMs restarts from 0 every stage/retry, so any stale absolute timestamp compared
        // against it (like the shot cooldown) must be reset too, or it reads as "still cooling
        // down" for a very long time (until clockMs catches back up to the old value).
        player.lastShotTimeMs = -100000;
        freezeTimer = -1;
        powerUpTimer = -1;
        prevFrameTimerMs = 0;
        enemiesCount = 0;
        scrollOn = true;
        revealedExtraRows = 0;
    }
};
