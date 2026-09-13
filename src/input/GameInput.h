#pragma once

// Abstracts the physical input source (keyboard, virtual d-pad/button, gamepad) into the four
// directions + fire.
class GameInput {
public:
    virtual ~GameInput() = default;
    virtual bool isUp() const = 0;
    virtual bool isDown() const = 0;
    virtual bool isLeft() const = 0;
    virtual bool isRight() const = 0;
    virtual bool isFireHeld() const = 0;
};
