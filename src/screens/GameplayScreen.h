#pragma once

#include "Screen.h"
#include "../Engine.h"
#include "../input/CompositeInput.h"
#include "../input/KeyboardInput.h"
#include "../render/GameRenderer.h"

class App;

// Drives the per-stage state machine, mirroring km.bas start_game()/run_stage() but expressed as a
// non-blocking phase machine suitable for a render(delta)-per-frame callback model.
class GameplayScreen : public Screen {
public:
    GameplayScreen(App& app, int startStage, int difficulty);

    void render(float delta) override;
    void resize(int width, int height) override;

private:
    enum class Phase { STAGE_INTRO_WAIT, STAGE_RESTART_WAIT, RUNNING, GAME_OVER_WAIT, DONE };

    App& app_;
    Engine engine_;
    GameRenderer renderer_;
    KeyboardInput keyboard_;
    CompositeInput input_;

    Phase phase_;
    float waitElapsed_ = 0;
    static constexpr float TICK_S = Constants::GAME_TICK_MS / 1000.0f;
    float accumulator_ = 0;

    void beginStageIteration();
    void proceedToRunStage();
};
