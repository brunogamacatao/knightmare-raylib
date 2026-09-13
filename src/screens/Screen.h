#pragma once

// Common interface for the game's two screens (title menu, gameplay), mirroring libGDX's Screen.
class Screen {
public:
    virtual ~Screen() = default;
    virtual void render(float delta) = 0;
    virtual void resize(int width, int height) = 0;
};
