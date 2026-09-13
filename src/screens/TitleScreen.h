#pragma once

#include <array>
#include "raylib.h"
#include "Screen.h"
#include "../input/CompositeInput.h"
#include "../input/KeyboardInput.h"
#include "../render/Hud.h"

class App;

// Port of screen.inc's show_intro()/show_menu(): title card and the difficulty/stage selection menu.
class TitleScreen : public Screen {
public:
    explicit TitleScreen(App& app);

    void render(float delta) override;
    void resize(int width, int height) override;

private:
    struct MenuLayout {
        float tile = 0, titleY = 0, creditY = 0;
        std::array<float, 4> itemY{};
    };

    App& app_;
    Hud hud_;
    KeyboardInput keyboard_;
    CompositeInput input_;

    float introElapsed = 0;
    bool introDone = false;

    int item = 1; // 1..4, matches ITEMS index+1 for parity with the original's 1-based menu
    int difficulty = 1;
    int stage = 1;
    bool released = true;
    bool wasTouchDown_ = false;

    void drawIntro();
    void drawMenu();
    MenuLayout menuLayout() const;
    void drawMenuLine(int index, float y, float tile, const std::string& text);
    void drawCentered(const std::string& text, float y, float tile, Color tint = WHITE);
    void updateMenu();
    void handleMenuTouch();
    void activateMenuItem(int index);
};
