#pragma once

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include "raylib.h"
#include "Constants.h"
#include "MapData.h"
#include "Sprite.h"

struct Player;

// Central holder for all textures, sounds, music and map data loaded from the original assets.
class Assets {
public:
    Texture2D mapTiles{};
    Texture2D objTiles{};
    Texture2D bossTiles{};

    std::array<std::unique_ptr<MapData>, 10> stages; // index 1..9

    bool soundOn = true;

    Assets();
    ~Assets();
    Assets(const Assets&) = delete;
    Assets& operator=(const Assets&) = delete;

    Sprite obj(int tx, int ty, int tw, int th) const { return {&objTiles, tx, ty, tw, th}; }
    Sprite boss(int tx, int ty, int tw, int th) const { return {&bossTiles, tx, ty, tw, th}; }
    Sprite mapRegion(int tx, int ty, int tw, int th) const { return {&mapTiles, tx, ty, tw, th}; }

    Sprite playerRegion(const Player& p) const;
    Sprite shieldRegion(int shieldHits) const;

    // SFX identifiers mirroring play_sfx() cases in music.inc.
    void playSfx(const std::string& sfx);

    void playStageSong(int stage);
    void playBossSong(int stage);
    void playSong(const std::string& name);
    void stopMusic();

    // Must be called once per frame while a song is playing (raylib streams music from disk).
    void update();

private:
    std::unordered_map<std::string, Sound> sounds;
    std::unordered_map<std::string, Music> musicCache;
    bool musicPlaying = false;
    std::string currentMusicName;

    void playSongFile(const std::string& file);
};
