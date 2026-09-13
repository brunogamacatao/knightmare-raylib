#pragma once

#include <initializer_list>
#include <vector>
#include "GameInput.h"

// Combines multiple input sources with a logical OR, so keyboard and virtual controls can coexist.
// Stores non-owning pointers; sources must outlive this object.
class CompositeInput : public GameInput {
public:
    CompositeInput(std::initializer_list<GameInput*> sources) : sources_(sources) {}

    bool isUp() const override { for (auto* i : sources_) if (i->isUp()) return true; return false; }
    bool isDown() const override { for (auto* i : sources_) if (i->isDown()) return true; return false; }
    bool isLeft() const override { for (auto* i : sources_) if (i->isLeft()) return true; return false; }
    bool isRight() const override { for (auto* i : sources_) if (i->isRight()) return true; return false; }
    bool isFireHeld() const override { for (auto* i : sources_) if (i->isFireHeld()) return true; return false; }

private:
    std::vector<GameInput*> sources_;
};
