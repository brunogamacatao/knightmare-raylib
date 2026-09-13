#include "TitleScreen.h"

#include <algorithm>
#include "GameplayScreen.h"
#include "../App.h"

namespace {
const char* DIFFICULTY_NAMES[] = {"EASY", "NORMAL", "HARD"};
}

TitleScreen::TitleScreen(App& app)
    : app_(app), hud_(app.assets), input_({&keyboard_, &app.virtualControls}) {
    app_.assets.playSong("TITLE");
}

void TitleScreen::render(float delta) {
    if (!introDone) {
        introElapsed += delta;
        if (introElapsed > 1.6f) introDone = true;
    } else {
        updateMenu();
    }

    ClearBackground(Color{5, 5, 15, 255});
    if (!introDone) drawIntro(); else drawMenu();
    app_.virtualControls.render();
}

void TitleScreen::drawIntro() {
    float w = (float)GetScreenWidth(), h = (float)GetScreenHeight();
    float tile = std::min(w, h) * 0.09f;
    float t = std::min(1.0f, introElapsed / 1.6f);
    float lh = tile * 3; // logo height; the logo graphic itself isn't drawn (see original comment)
    float startY = -lh, endY = h / 2.0f - lh / 2.0f;
    float y = startY + (endY - startY) * t;
    const char* credit = "ORIGINAL GAME BY KONAMI(C) 1986";
    hud_.draw(credit, (w - hud_.width(credit, tile * 0.55f)) / 2.0f, y + lh, tile * 0.55f);
}

TitleScreen::MenuLayout TitleScreen::menuLayout() const {
    float w = (float)GetScreenWidth(), h = (float)GetScreenHeight();
    MenuLayout l;
    l.tile = std::min(w, h) * 0.075f;
    float y = h * 0.05f;
    l.titleY = y; y += l.tile * 1.6f;
    l.creditY = y; y += l.tile * 1.1f;
    y += l.tile * 0.7f;
    y += l.tile * 0.7f;
    y += l.tile * 0.6f;
    y += l.tile * 2.2f;
    for (int i = 0; i < 4; i++) { l.itemY[i] = y; y += l.tile * 1.6f; }
    return l;
}

void TitleScreen::drawMenu() {
    MenuLayout l = menuLayout();
    drawCentered("KNIGHTMARE", l.titleY, l.tile * 1.6f);
    drawCentered("ORIGINAL GAME BY KONAMI(C) 1986", l.creditY, l.tile * 0.5f);

    drawMenuLine(0, l.itemY[0], l.tile, std::string("DIFFICULTY: ") + DIFFICULTY_NAMES[difficulty]);
    drawMenuLine(1, l.itemY[1], l.tile, "STAGE: " + std::string(stage < 10 ? "0" : "") + std::to_string(stage));
    drawMenuLine(2, l.itemY[2], l.tile, "START GAME");
    drawMenuLine(3, l.itemY[3], l.tile, "QUIT");
}

void TitleScreen::drawMenuLine(int index, float y, float tile, const std::string& text) {
    bool selected = item - 1 == index;
    Color tint = selected ? Color{255, 217, 51, 255} : WHITE;
    drawCentered(text, y, tile * (selected ? 0.95f : 0.85f), tint);
}

void TitleScreen::drawCentered(const std::string& text, float y, float tile, Color tint) {
    float w = (float)GetScreenWidth();
    float maxW = w * 0.94f;
    float tw = hud_.width(text, tile);
    if (tw > maxW) tile *= maxW / tw;
    tw = hud_.width(text, tile);
    hud_.draw(text, (w - tw) / 2.0f, y, tile, tint);
}

void TitleScreen::updateMenu() {
    handleMenuTouch();

    bool up = input_.isUp(), down = input_.isDown(), left = input_.isLeft(), right = input_.isRight(), fire = input_.isFireHeld();
    bool any = up || down || left || right || fire;
    if (!any) { released = true; return; }
    if (!released) return;
    released = false;

    if (up) item = std::max(1, item - 1);
    else if (down) item = std::min(4, item + 1);
    else if (right) {
        if (item == 1) difficulty = std::min(2, difficulty + 1);
        else if (item == 2) stage = std::min(8, stage + 1);
    } else if (left) {
        if (item == 1) difficulty = std::max(0, difficulty - 1);
        else if (item == 2) stage = std::max(1, stage - 1);
    } else if (fire) {
        if (item == 3) {
            app_.setScreen(std::make_unique<GameplayScreen>(app_, stage, difficulty));
        } else if (item == 4) {
            app_.requestQuit();
        }
    }
}

// Lets a mouse click or a finger tap select/activate a menu line directly, instead of requiring
// the d-pad + fire to reach it first.
void TitleScreen::handleMenuTouch() {
    bool touchDownNow = GetTouchPointCount() > 0;
    bool justPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    Vector2 pos = GetMousePosition();
    if (!justPressed && touchDownNow && !wasTouchDown_) {
        justPressed = true;
        pos = GetTouchPosition(0);
    }
    wasTouchDown_ = touchDownNow;
    if (!justPressed) return;

    if (app_.virtualControls.isInControlsBar(pos.y)) return;

    MenuLayout l = menuLayout();
    float lineHeight = l.tile * 1.6f;
    for (int i = 0; i < 4; i++) {
        if (pos.y >= l.itemY[i] - lineHeight * 0.15f && pos.y <= l.itemY[i] + lineHeight * 0.85f) {
            activateMenuItem(i);
            return;
        }
    }
}

void TitleScreen::activateMenuItem(int index) {
    item = index + 1;
    switch (index) {
        case 0: difficulty = (difficulty + 1) % 3; break;
        case 1: stage = stage % 8 + 1; break;
        case 2: app_.setScreen(std::make_unique<GameplayScreen>(app_, stage, difficulty)); break;
        case 3: app_.requestQuit(); break;
    }
}

void TitleScreen::resize(int width, int height) {
    app_.virtualControls.resize(width, height);
}
