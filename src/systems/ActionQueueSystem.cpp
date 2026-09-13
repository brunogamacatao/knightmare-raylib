#include "ActionQueueSystem.h"

#include "../Engine.h"

QueuedAction* ActionQueueSystem::newAction(int execCount, int actionId, long long delayMs) {
    auto a = std::make_unique<QueuedAction>();
    QueuedAction* raw = a.get();
    engine_.state.queue.push_back(std::move(a));
    raw->execCount = execCount;
    raw->actionId = actionId;
    raw->nextExecMs = engine_.state.clockMs + delayMs;
    raw->delayMs = delayMs;
    return raw;
}

void ActionQueueSystem::enqueue(int execCount, int actionId, float gpr1, float gpr2, float gpr3, long long delayMs) {
    QueuedAction* a = newAction(execCount, actionId, delayMs);
    a->gpr1 = gpr1; a->gpr2 = gpr2; a->gpr3 = gpr3;
}

void ActionQueueSystem::enqueueBlock(int execCount, int actionId, Block* block, long long delayMs) {
    newAction(execCount, actionId, delayMs)->blockRef = block;
}

void ActionQueueSystem::process() {
    GameState& s = engine_.state;
    // Index-bounded, not a range-for: actions processed below can enqueue further actions (e.g.
    // spawning an object with objCfg > 0), appending to this very deque. Capturing the size up
    // front means any such append is simply not visited until next tick - see the container
    // comment on GameState::queue.
    size_t n = s.queue.size();
    for (size_t i = 0; i < n; i++) {
        QueuedAction& a = *s.queue[i];
        if (a.isFree()) continue;
        if (s.clockMs < a.nextExecMs) continue;

        switch (a.actionId) {
            case 22: // Blocks
                engine_.mapSystem.replaceBlock(a.blockRef);
                break;
            case 36: // Shield
                engine_.objectSystem.spawnShield();
                break;
            case 90: // Replace boss skin - always the boss's current body.
                engine_.bossSystem.showBossBody(s.boss.body, (int)a.gpr2);
                break;
            case 49: // Open terrain
                engine_.objectSystem.spawnObject(49, a.gpr1, a.gpr2, 0);
                break;
            case 100: // Boss death blinks the screen
                s.blinkActive = true;
                s.blinkGray = a.execCount % 2 != 0;
                if (a.execCount == 1) engine_.bossSystem.destroyBoss();
                break;
            case 101: // Move player to portal
                engine_.assets.playSong("STAGE_INTRO");
                engine_.objectSystem.movePlayerToPortal();
                break;
            default: // Objects and enemies
                engine_.objectSystem.spawnObject(a.actionId, a.gpr1, a.gpr2, (int)a.gpr3);
                break;
        }

        a.execCount--;
        a.nextExecMs = s.clockMs + a.delayMs;
        if (a.execCount <= 0) {
            a.execCount = 0;
            if (a.actionId == 100) s.blinkActive = false;
        }
    }
}

void ActionQueueSystem::clear() {
    engine_.state.queue.clear();
}
