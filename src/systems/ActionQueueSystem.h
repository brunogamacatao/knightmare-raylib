#pragma once

struct Block;

// Port of queue.inc: a delayed/repeating action queue used to stagger enemy spawns and effects.
class ActionQueueSystem {
public:
    explicit ActionQueueSystem(class Engine& engine) : engine_(engine) {}

    // Unbounded: appends a fresh QueuedAction rather than scanning a fixed-size slot table.
    void enqueue(int execCount, int actionId, float gpr1, float gpr2, float gpr3, long long delayMs);

    // Same as enqueue(), for action 22 (replace block), which needs to act on a specific block by
    // reference rather than an index - blocks can be removed/compacted between the enqueue and the
    // action actually running.
    void enqueueBlock(int execCount, int actionId, Block* block, long long delayMs);

    void process();
    void clear();

private:
    Engine& engine_;

    struct QueuedAction* newAction(int execCount, int actionId, long long delayMs);
};
