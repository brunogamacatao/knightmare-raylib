#include "GameplayScreen.h"

#include "raylib.h"
#include "TitleScreen.h"
#include "../App.h"

GameplayScreen::GameplayScreen(App& app, int startStage, int difficulty)
    : app_(app), engine_(app.assets), renderer_(engine_, app.assets), input_({&keyboard_, &app.virtualControls}) {
    engine_.input = &input_;

    GameState& s = engine_.state;
    s.stage = startStage;
    s.difficulty = difficulty;
    s.soundOn = true;
    s.hiscore = app_.hiscore;

    engine_.initPlayer(2);
    s.row = Constants::MAP_ROWS_0;
    s.score = 0;
    beginStageIteration();
}

void GameplayScreen::beginStageIteration() {
    GameState& s = engine_.state;
    s.clockMs = 0;
    waitElapsed_ = 0;

    if (s.stage == 9) {
        s.centerMessage = "YOU HAVE BEATEN ALL DEMONS !";
    } else {
        s.centerMessage = "STAGE " + std::string(s.stage < 10 ? "0" : "") + std::to_string(s.stage);
    }

    if (s.player.state == Player::READY_NEXT_STAGE) {
        phase_ = Phase::STAGE_INTRO_WAIT;
    } else if (s.player.state == Player::RESTART_STAGE) {
        if (s.player.lives < 0) {
            phase_ = Phase::GAME_OVER_WAIT;
            s.centerMessage = "GAME OVER";
            app_.assets.playSong("GAME_OVER");
            return;
        }
        engine_.mapSystem.calculateStartRow();
        phase_ = Phase::STAGE_RESTART_WAIT;
    } else if (s.player.state > 10) {
        phase_ = Phase::DONE;
    } else {
        proceedToRunStage();
    }
}

void GameplayScreen::proceedToRunStage() {
    GameState& s = engine_.state;
    s.centerMessage = "";
    if (s.player.state == Player::READY_NEXT_STAGE) {
        if (s.stage == 9) {
            s.player.powerUp = Player::PWR_NONE;
            s.player.shield = 0;
            s.player.state = Player::CUTSCENE_STILL;
        } else {
            s.player.state = Player::PLAYING;
        }
    } else if (s.player.state == Player::RESTART_STAGE) {
        s.player.state = Player::PLAYING;
    }
    engine_.initStage();
    phase_ = Phase::RUNNING;
}

void GameplayScreen::render(float delta) {
    GameState& s = engine_.state;

    switch (phase_) {
        case Phase::STAGE_INTRO_WAIT:
            waitElapsed_ += delta * 1000;
            if (waitElapsed_ > 4000) proceedToRunStage();
            break;
        case Phase::STAGE_RESTART_WAIT:
            waitElapsed_ += delta * 1000;
            if (waitElapsed_ > 2000) proceedToRunStage();
            break;
        case Phase::RUNNING: {
            accumulator_ += delta;
            int steps = 0;
            while (accumulator_ >= TICK_S && steps < 12) {
                engine_.tick();
                accumulator_ -= TICK_S;
                steps++;
            }
            if (s.hiscore > app_.hiscore) app_.hiscore = s.hiscore;
            if (engine_.stageFinished()) {
                engine_.objectSystem.destroyAll();
                if (s.player.state == Player::READY_NEXT_STAGE) {
                    s.stage++;
                    s.row = s.stage == 9 ? 0 : Constants::MAP_ROWS_0;
                }
                beginStageIteration();
            }
            break;
        }
        case Phase::GAME_OVER_WAIT:
            waitElapsed_ += delta * 1000;
            if (waitElapsed_ > 5200) {
                app_.assets.stopMusic();
                app_.setScreen(std::make_unique<TitleScreen>(app_));
                return;
            }
            break;
        case Phase::DONE:
            app_.setScreen(std::make_unique<TitleScreen>(app_));
            return;
    }

    ClearBackground(BLACK);
    renderer_.render();
    app_.virtualControls.render();
}

void GameplayScreen::resize(int width, int height) {
    renderer_.resize(width, height);
    app_.virtualControls.resize(width, height);
}
