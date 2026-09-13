#pragma once

#include "raylib.h"
#include "GameInput.h"

// Keyboard input for desktop testing (arrow keys / WASD + space or Z to fire).
class KeyboardInput : public GameInput {
public:
    bool isUp() const override { return IsKeyDown(KEY_UP) || IsKeyDown(KEY_W); }
    bool isDown() const override { return IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S); }
    bool isLeft() const override { return IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A); }
    bool isRight() const override { return IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D); }
    bool isFireHeld() const override { return IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_Z); }
};
