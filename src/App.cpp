#include "App.h"

#include "raylib.h"
#include "screens/TitleScreen.h"

App::App() : virtualControls(assets) {
    currentScreen_ = std::make_unique<TitleScreen>(*this);
}

void App::setScreen(std::unique_ptr<Screen> screen) {
    pendingScreen_ = std::move(screen);
}

void App::resize(int width, int height) {
    if (currentScreen_) currentScreen_->resize(width, height);
    virtualControls.resize(width, height);
}

void App::step(float delta) {
    int w = GetScreenWidth(), h = GetScreenHeight();
    if (w != lastWidth_ || h != lastHeight_) {
        lastWidth_ = w;
        lastHeight_ = h;
        resize(w, h);
    }

    assets.update();
    virtualControls.update();
    if (currentScreen_) currentScreen_->render(delta);

    if (pendingScreen_) {
        currentScreen_ = std::move(pendingScreen_);
        resize(lastWidth_, lastHeight_);
    }
}
