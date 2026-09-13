#pragma once

struct Block;

// Direct port of one slot of g_actions_queue(): 0:execCount 1:actionId 2:gpr1 3:gpr2 4:gpr3 5:nextExecMs 6:delayMs.
struct QueuedAction {
    int execCount = 0; // 0 = free slot
    int actionId = 0;
    float gpr1 = 0, gpr2 = 0, gpr3 = 0;
    long long nextExecMs = 0;
    long long delayMs = 0;
    // Only used by action 22 (replace block): the block to act on, passed by reference instead of
    // a pool index since blocks can be removed/compacted between enqueue and processing.
    // Non-owning: blocks are owned by GameState's vector of unique_ptr.
    Block* blockRef = nullptr;

    bool isFree() const { return execCount == 0; }
};
