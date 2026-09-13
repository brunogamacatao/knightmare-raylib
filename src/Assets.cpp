#include "Assets.h"

#include "entity/Player.h"

namespace {
constexpr const char* SFX_NAMES[] = {
    "boss_hit", "boss_kill", "boss_shield", "block_hit", "block_open", "shot_boomerang",
    "chrystal", "enemy_kill", "shot_fire_arrow", "shot_flame", "freeze_tick", "helmet",
    "kill_all_enemies", "new_life", "player_death", "points", "power_up_ending",
    "power_up_hit", "red_death", "shield", "shot", "split", "skeleton_restored",
    "shot_sword", "terrain", "pause"
};

const std::array<std::string, 10> MUSIC = {
    "01-gamestart", "02-bgm1", "04-bgm2", "06-prince_of_maresia", "08-tavern_funk",
    "10-another_level", "12-xakt", "14-positive_trek", "16-last_hurdle", "25-ending"
};
const std::array<std::string, 10> BOSS_MUSIC = {
    "", "03-boss1", "05-boss2", "07-bat_rock", "09-blind_panic", "11-more_boss",
    "13-under_his_eyes", "15-boss_fronty_ii", "17-manbow_reprise", "25-ending"
};
} // namespace

Assets::Assets() {
    mapTiles = LoadTexture("assets/tiles/maps.png");
    objTiles = LoadTexture("assets/tiles/objects.png");
    bossTiles = LoadTexture("assets/tiles/bosses.png");
    SetTextureFilter(mapTiles, TEXTURE_FILTER_POINT);
    SetTextureFilter(objTiles, TEXTURE_FILTER_POINT);
    SetTextureFilter(bossTiles, TEXTURE_FILTER_POINT);

    for (int i = 1; i <= 9; i++) {
        stages[i] = std::make_unique<MapData>("assets/maps/stage" + std::to_string(i) + ".map");
    }

    for (const char* n : SFX_NAMES) {
        sounds[n] = LoadSound((std::string("assets/sfx/") + n + ".wav").c_str());
    }
}

Assets::~Assets() {
    for (auto& [name, sound] : sounds) UnloadSound(sound);
    for (auto& [name, music] : musicCache) UnloadMusicStream(music);
    UnloadTexture(mapTiles);
    UnloadTexture(objTiles);
    UnloadTexture(bossTiles);
}

Sprite Assets::playerRegion(const Player& p) const {
    bool leftLeg = p.state != Player::DEAD && (static_cast<int>(p.animCounter)) % 10 < 5;
    int x = (leftLeg ? Constants::PLAYER_SKIN1_X_L : Constants::PLAYER_SKIN1_X_R) + p.animOffsetX;
    return obj(x, Constants::PLAYER_SKIN_Y, Constants::TILE_SIZE_X2, Constants::TILE_SIZE_X2);
}

Sprite Assets::shieldRegion(int shieldHits) const {
    const Constants::ObjTile& t = Constants::OBJ_TILE[36];
    int max6 = Constants::SHIELD_MAX_HITS / 6;
    int offsetX = shieldHits <= max6 ? Constants::TILE_SIZE_X4 : (shieldHits <= max6 * 2 ? Constants::TILE_SIZE_X2 : 0);
    return obj(t.x + offsetX, t.y, t.w, t.h);
}

void Assets::playSfx(const std::string& sfx) {
    if (!soundOn || sfx.empty()) return;
    static const std::unordered_map<std::string, std::string> map = {
        {"BOSS_HIT", "boss_hit"}, {"BOSS_KILL", "boss_kill"}, {"BOSS_SHIELD", "boss_shield"},
        {"BLOCK_HIT", "block_hit"}, {"BLOCK_OPEN", "block_open"}, {"BOOMERANG", "shot_boomerang"},
        {"CHRYSTAL", "chrystal"}, {"ENEMY_KILL", "enemy_kill"}, {"FIRE_ARROW", "shot_fire_arrow"},
        {"FLAME", "shot_flame"}, {"FREEZE_TICK", "freeze_tick"}, {"HELMET", "helmet"},
        {"KILL_ALL_ENEMIES", "kill_all_enemies"}, {"NEW_LIFE", "new_life"}, {"PLAYER_DEATH", "player_death"},
        {"POINTS", "points"}, {"POWER_UP_ENDING", "power_up_ending"}, {"POWER_UP_HIT", "power_up_hit"},
        {"RED_DEATH", "red_death"}, {"SHIELD", "shield"}, {"SHOT", "shot"}, {"SPLIT", "split"},
        {"SKELETON_RESTORED", "skeleton_restored"}, {"SWORD", "shot_sword"}, {"TERRAIN", "terrain"},
    };
    auto it = map.find(sfx);
    if (it == map.end()) return;
    auto sIt = sounds.find(it->second);
    if (sIt != sounds.end()) PlaySound(sIt->second);
}

void Assets::playStageSong(int stage) { playSong(MUSIC[stage]); }

void Assets::playBossSong(int stage) {
    const std::string& song = BOSS_MUSIC[stage];
    if (song.empty()) return;
    playSong(song);
}

void Assets::playSong(const std::string& name) {
    std::string file;
    if (name == "STAGE_INTRO") file = MUSIC[0];
    else if (name == "SILENCE") file = "26-silence";
    else if (name == "TITLE") file = "24-title";
    else if (name == "GAME_OVER") file = "20-gameover";
    else file = name;
    playSongFile(file);
}

void Assets::playSongFile(const std::string& file) {
    stopMusic();
    if (!soundOn) return;
    currentMusicName = file;
    auto it = musicCache.find(file);
    if (it == musicCache.end()) {
        Music m = LoadMusicStream(("assets/music/" + file + ".ogg").c_str());
        it = musicCache.emplace(file, m).first;
    }
    it->second.looping = true;
    PlayMusicStream(it->second);
    musicPlaying = true;
}

void Assets::stopMusic() {
    if (musicPlaying) {
        auto it = musicCache.find(currentMusicName);
        if (it != musicCache.end()) StopMusicStream(it->second);
        musicPlaying = false;
        currentMusicName.clear();
    }
}

void Assets::update() {
    if (musicPlaying) {
        auto it = musicCache.find(currentMusicName);
        if (it != musicCache.end()) UpdateMusicStream(it->second);
    }
}
