#pragma once

#include <vector>
#include "raylib.h"
#include "GameInput.h"
#include "../Assets.h"
#include "../render/Hud.h"

// On-screen touch controls: the bottom control bar is split in half. The left half is a
// "floating"/relative joystick - direction is derived purely from how far the finger has dragged
// away from wherever it first touched down (dx/dy from that origin), not from proximity to any
// fixed on-screen pad, so it works no matter where on that half the player presses. The right half
// is a plain fire button. Both halves are overlaid with a translucent legend (blue arrows on the
// left, mimicking the MSX "Expert Gradiente" cursor-key cluster; a red "FIRE" panel on the right)
// that brightens while active.
//
// Also responds to the mouse on desktop (treated as one extra synthetic touch point while the left
// button is held), matching the original libGDX port where mouse and touch share one input API.
class VirtualControls : public GameInput {
public:
    explicit VirtualControls(const Assets& assets);

    void resize(int width, int height);

    // True if the given screen-space (raylib convention: y down, origin top-left) coordinate falls
    // inside the reserved controls bar at the bottom of the screen.
    bool isInControlsBar(float screenY) const;

    void update();
    void render() const;

    bool isUp() const override { return up_; }
    bool isDown() const override { return down_; }
    bool isLeft() const override { return left_; }
    bool isRight() const override { return right_; }
    bool isFireHeld() const override { return fire_; }

private:
    static constexpr int NO_POINTER = -1000000; // sentinel "no id", distinct from any real touch/mouse id
    static constexpr int MOUSE_ID = -2;         // sentinel id for the synthetic mouse pointer

    const Hud hud_;

    int screenW_ = 0, screenH_ = 0;
    float barHeight_ = 0;
    float halfW_ = 0;
    float moveRadius_ = 0, deadZone_ = 0;

    std::vector<int> previousIds_; // ids that were down last frame, for edge (justDown) detection
    int movePointer_ = NO_POINTER;
    int firePointer_ = NO_POINTER;
    float originX_ = 0, originY_ = 0;
    float dragDx_ = 0, dragDy_ = 0;

    bool up_ = false, down_ = false, left_ = false, right_ = false, fire_ = false;

    void processPoint(int id, float tx, float ty, std::vector<int>& currentIds);
    void applyDrag();
    void drawMoveLegend(bool active) const;
    void drawFireLegend(bool active) const;
    void drawArrowCell(float cx, float cy, float size, int dir, bool active) const;
};
