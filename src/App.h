#pragma once

#include <memory>
#include "Assets.h"
#include "input/VirtualControls.h"
#include "screens/Screen.h"

// Entry point shared by every platform launcher (equivalent to the original libGDX port's
// KnightmareGame). Owns everything that must survive a screen switch: assets, the shared virtual
// on-screen controls, and the running high score.
class App {
public:
    Assets assets;
    VirtualControls virtualControls;
    int hiscore = 0;

    App();

    // Screen switches are deferred to the start of the next frame (see step()) rather than applied
    // immediately: setScreen() is normally called from inside the current screen's own render(), and
    // replacing (and destroying) that screen object while its render() call is still on the stack
    // would be a use-after-free.
    void setScreen(std::unique_ptr<Screen> screen);

    void resize(int width, int height);
    void step(float delta);

    bool quitRequested() const { return quitRequested_; }
    void requestQuit() { quitRequested_ = true; }

private:
    std::unique_ptr<Screen> currentScreen_;
    std::unique_ptr<Screen> pendingScreen_;
    int lastWidth_ = 0, lastHeight_ = 0;
    bool quitRequested_ = false;
};
