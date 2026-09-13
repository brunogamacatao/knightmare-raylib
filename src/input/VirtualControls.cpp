#include "VirtualControls.h"

#include <algorithm>
#include <cmath>
#include "../Constants.h"
#include "../Platform.h"

namespace {
Color colorf(float r, float g, float b, float a) {
    return Color{
        (unsigned char)std::lround(r * 255), (unsigned char)std::lround(g * 255),
        (unsigned char)std::lround(b * 255), (unsigned char)std::lround(a * 255)
    };
}
constexpr int ARROW_UP = 0, ARROW_DOWN = 1, ARROW_LEFT = 2, ARROW_RIGHT = 3;
} // namespace

VirtualControls::VirtualControls(const Assets& assets) : hud_(assets) {}

void VirtualControls::resize(int width, int height) {
    screenW_ = width;
    screenH_ = height;
    // Desktop has no bar at all: no on-screen touch controls there, keyboard only (see
    // update()/render() below). On mobile this is purely a translucent overlay drawn on top of the
    // game (GameRenderer never reserves space for it - see its own resize()), not empty space.
    barHeight_ = Platform::kMobile ? height * Constants::VIRTUAL_CONTROLS_HEIGHT_FRACTION : 0.0f;
    halfW_ = width / 2.0f;
    moveRadius_ = std::min(width, height) * 0.16f;
    deadZone_ = moveRadius_ * 0.22f;
}

bool VirtualControls::isInControlsBar(float screenY) const {
    return screenY >= screenH_ - barHeight_;
}

void VirtualControls::processPoint(int id, float tx, float ty, std::vector<int>& currentIds) {
    currentIds.push_back(id);
    bool justDown = std::find(previousIds_.begin(), previousIds_.end(), id) == previousIds_.end();

    if (justDown && ty >= screenH_ - barHeight_) {
        if (tx < halfW_ && movePointer_ == NO_POINTER) {
            movePointer_ = id;
            originX_ = tx;
            originY_ = ty;
        } else if (tx >= halfW_ && firePointer_ == NO_POINTER) {
            firePointer_ = id;
        }
    }

    if (movePointer_ == id) {
        dragDx_ = tx - originX_;
        dragDy_ = ty - originY_;
        applyDrag();
    } else if (firePointer_ == id) {
        fire_ = true;
    }
}

void VirtualControls::update() {
    up_ = down_ = left_ = right_ = false;
    fire_ = false;
    // Desktop has no on-screen touch controls at all - keyboard only (see main.cpp/Platform.h).
    if constexpr (!Platform::kMobile) return;

    std::vector<int> currentIds;

    int touchCount = GetTouchPointCount();
    for (int i = 0; i < touchCount; i++) {
        int id = GetTouchPointId(i);
        Vector2 p = GetTouchPosition(i);
        processPoint(id, p.x, p.y, currentIds);
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 p = GetMousePosition();
        processPoint(MOUSE_ID, p.x, p.y, currentIds);
    }

    if (movePointer_ != NO_POINTER && std::find(currentIds.begin(), currentIds.end(), movePointer_) == currentIds.end()) {
        movePointer_ = NO_POINTER;
    }
    if (firePointer_ != NO_POINTER && std::find(currentIds.begin(), currentIds.end(), firePointer_) == currentIds.end()) {
        firePointer_ = NO_POINTER;
    }
    if (movePointer_ == NO_POINTER) { dragDx_ = 0; dragDy_ = 0; }

    previousIds_ = std::move(currentIds);
}

void VirtualControls::applyDrag() {
    if (dragDx_ > deadZone_) right_ = true;
    if (dragDx_ < -deadZone_) left_ = true;
    if (dragDy_ < -deadZone_) up_ = true;   // raylib is y-down: "up" means the finger moved toward smaller y
    if (dragDy_ > deadZone_) down_ = true;
}

void VirtualControls::render() const {
    if constexpr (!Platform::kMobile) return;

    bool moveActive = movePointer_ != NO_POINTER;

    drawMoveLegend(moveActive);
    drawFireLegend(fire_);

    if (moveActive) {
        float len = std::sqrt(dragDx_ * dragDx_ + dragDy_ * dragDy_);
        float kx = dragDx_, ky = dragDy_;
        if (len > moveRadius_) { kx = kx / len * moveRadius_; ky = ky / len * moveRadius_; }
        DrawCircleV({originX_, originY_}, moveRadius_ * 0.5f, colorf(1, 1, 1, 0.25f));
        DrawCircleV({originX_ + kx, originY_ + ky}, moveRadius_ * 0.32f, colorf(1, 1, 1, 0.55f));
    }

    const char* fireText = "FIRE";
    float tile = barHeight_ * 0.16f;
    float tw = hud_.width(fireText, tile);
    float barCenterY = screenH_ - barHeight_ / 2.0f;
    hud_.draw(fireText, halfW_ + (halfW_ - tw) / 2.0f, barCenterY - tile / 2.0f, tile);
}

void VirtualControls::drawMoveLegend(bool active) const {
    float colW = halfW_ / 3.0f;
    float barTop = screenH_ - barHeight_;
    float cy = barTop + barHeight_ / 2.0f;

    DrawRectangle(0, (int)barTop, (int)halfW_, (int)barHeight_, colorf(0.15f, 0.35f, 0.9f, active ? 0.42f : 0.24f));

    float arrowSize = std::min(colW, barHeight_ / 2.0f) * 0.6f;
    drawArrowCell(colW * 0.5f, cy, arrowSize, ARROW_LEFT, left_);
    drawArrowCell(halfW_ - colW * 0.5f, cy, arrowSize, ARROW_RIGHT, right_);
    drawArrowCell(halfW_ * 0.5f, cy - barHeight_ * 0.22f, arrowSize, ARROW_UP, up_);
    drawArrowCell(halfW_ * 0.5f, cy + barHeight_ * 0.22f, arrowSize, ARROW_DOWN, down_);
}

void VirtualControls::drawArrowCell(float cx, float cy, float size, int dir, bool active) const {
    Color col = colorf(0.85f, 0.92f, 1.0f, active ? 1.0f : 0.55f);
    float h = size * 0.9f, w = size * 0.9f;
    // All four triangles must wind the same way (clockwise here, matching LEFT/RIGHT below) - the
    // UP/DOWN cases used the opposite winding, which rendered fine on desktop OpenGL (no backface
    // culling for 2D there) but is invisible on some Android/GLES drivers that do cull it.
    switch (dir) {
        case ARROW_UP:
            DrawTriangle({cx - w / 2, cy + h / 2}, {cx + w / 2, cy + h / 2}, {cx, cy - h / 2}, col);
            break;
        case ARROW_DOWN:
            DrawTriangle({cx - w / 2, cy - h / 2}, {cx, cy + h / 2}, {cx + w / 2, cy - h / 2}, col);
            break;
        case ARROW_LEFT:
            DrawTriangle({cx + w / 2, cy + h / 2}, {cx + w / 2, cy - h / 2}, {cx - w / 2, cy}, col);
            break;
        case ARROW_RIGHT:
            DrawTriangle({cx - w / 2, cy - h / 2}, {cx - w / 2, cy + h / 2}, {cx + w / 2, cy}, col);
            break;
    }
}

void VirtualControls::drawFireLegend(bool active) const {
    float barTop = screenH_ - barHeight_;
    DrawRectangle((int)halfW_, (int)barTop, (int)halfW_, (int)barHeight_, colorf(0.9f, 0.15f, 0.1f, active ? 0.55f : 0.32f));
}
