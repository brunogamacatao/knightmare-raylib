#pragma once

struct GameObj;

// Port of power_ups.inc.
class PowerUpSystem {
public:
    explicit PowerUpSystem(class Engine& engine) : engine_(engine) {}

    void changeWeapon(GameObj& o);
    void powerUp(GameObj& o);
    void increasePlayerSpeed();
    void incrementScore(int points);
    void updateLife(int value);
    bool isSuperWeapon() const;

private:
    Engine& engine_;
};
