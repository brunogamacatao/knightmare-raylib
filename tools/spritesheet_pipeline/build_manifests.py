#!/usr/bin/env python3
"""Transcribe the hardcoded tile rectangles from the C++ source into manifest JSON files.

Why this exists instead of just auto-detecting sprite boxes from the PNG pixels:
the renderer (src/render/Draw.h) samples sprites using raw pixel rectangles it
gets from src/Constants.h (OBJ_TILE), src/systems/BossSystem.cpp and
src/render/Hud.cpp. Those rectangles are sometimes intentionally padded
(e.g. extra transparent margin around a walk-cycle frame so the character's
feet stay anchored across frames) -- an ink/alpha bounding-box detector would
crop *tighter* than that and silently shift/clip the tile. For the "swap the
spritesheet filename, nothing else changes" goal to actually hold once the
sheet is restyled + upscaled, the atlas positions used to cut and reassemble
tiles must be pixel-identical (before scaling) to what the game already
samples today.

So: this script mirrors those C++ formulas in Python (each block below cites
the source file/lines it was transcribed from) and writes out one manifest
JSON per sheet. Re-run it whenever the corresponding C++ tables change.
Run extract_atlas.py --mode manifest afterwards to turn a manifest into the
standard atlas JSON/CSV that restyle_spritesheet.py consumes.

Output: source_manifests/objects_manifest.json, source_manifests/bosses_manifest.json
(maps.png doesn't need a manifest -- it's a plain uniform grid, see
extract_atlas.py --mode grid, which already mirrors GameRenderer.cpp's
tx/ty formula exactly.)
"""
import json
from pathlib import Path

TILE = 8
OUT_DIR = Path(__file__).parent / "source_manifests"


def rect(name, x, y, w, h, source):
    return {"name": name, "x": x, "y": y, "w": w, "h": h, "source": source}


def build_objects_manifest():
    entries = []

    # --- src/Constants.h buildObjTiles(): OBJ_TILE[id] = {x, y, w, h, dataId} ---
    # id 50 (boss eyes) is sampled from bosses.png, not objects.png -- see build_bosses_manifest().
    OBJ_TILE = {
        1: (1, 17, 14, 14, "Blob"), 2: (32, 16, 16, 11, "Bat"), 3: (32, 16, 16, 11, "Bat wave"),
        4: (64, 16, 16, 16, "Knight"), 5: (113, 17, 14, 14, "Cloud"), 6: (144, 16, 16, 16, "Blue demon"),
        7: (176, 16, 16, 16, "Skeleton"), 8: (240, 17, 16, 15, "Demon"), 9: (16, 136, 15, 16, "Death ghost"),
        10: (64, 136, 16, 16, "Zombie"), 11: (16, 120, 16, 16, "Ghost"), 12: (209, 2, 13, 13, "Yellow thing"),
        13: (224, 1, 15, 14, "Red thing"), 14: (32, 16, 16, 11, "Bat reverse"), 15: (0, 120, 16, 16, "Sorcerer"),
        16: (32, 136, 15, 16, "Red death ghost"),
        20: (0, 64, 16, 16, "Weapon crystal"), 21: (0, 80, 16, 16, "Power up crystal"), 22: (0, 104, 16, 16, "Blocks"),
        23: (22, 38, 4, 4, "Dot bullet"), 24: (35, 35, 10, 10, "Energy ray"), 25: (50, 37, 12, 6, "Bone"),
        26: (66, 35, 11, 11, "Shot 26"), 27: (85, 34, 6, 12, "Shot 27"), 28: (99, 35, 9, 9, "White explosion"),
        29: (133, 34, 5, 14, "Arrow"), 30: (145, 38, 13, 5, "Shot 30"), 31: (163, 35, 11, 11, "Shot 31"),
        32: (177, 34, 13, 13, "Axe"), 33: (48, 120, 16, 16, "Scythe"), 34: (112, 104, 16, 16, "Fire"),
        35: (112, 120, 16, 16, "Big fire"), 36: (136, 48, 16, 7, "Shield"),
        37: (200, 98, 40, 6, "Boss shadow"), 38: (184, 49, 24, 6, "Big shadow"), 39: (0, 33, 16, 6, "Shadow"),
        40: (209, 50, 5, 13, "Arrow (player)"), 41: (210, 66, 12, 13, "Twin arrows"),
        42: (218, 54, 4, 10, "Triple flames"), 43: (209, 35, 5, 10, "Boomerang"), 44: (218, 37, 10, 5, "Shot 44"),
        45: (225, 48, 6, 16, "Sword"), 46: (226, 64, 13, 16, "Double sword"), 47: (232, 49, 8, 14, "Fire arrow"),
        48: (184, 136, 32, 8, "Bridge"), 49: (240, 192, 16, 8, "Terrain"), 51: (0, 136, 16, 24, "Princess"),
    }
    for obj_id, (x, y, w, h, label) in OBJ_TILE.items():
        entries.append(rect(f"obj[{obj_id}] {label}", x, y, w, h, "Constants.h:buildObjTiles"))

    # --- src/Assets.cpp shieldRegion(): base = OBJ_TILE[36], offsetX in {0, TILE*2, TILE*4} ---
    base_x, base_y, base_w, base_h = 136, 48, 16, 7
    for offset in (0, TILE * 2, TILE * 4):
        entries.append(rect(f"shield offsetX={offset}", base_x + offset, base_y, base_w, base_h, "Assets.cpp:shieldRegion"))

    # --- src/Assets.cpp playerRegion() + src/systems/ObjectSystem.cpp animatePlayer() ---
    # x = (PLAYER_SKIN1_X_L=0 | PLAYER_SKIN1_X_R=16) + animOffsetX, y = PLAYER_SKIN_Y = 0, size 16x16.
    # animOffsetX is 0 while idle/moving, TILE*2*powerUp for powerUp in {2 (INVISIBLE), 4 (INVINCIBLE)},
    # or PLAYER_DEATH_ANIM[i]*TILE for the death animation (distinct frame values: 0,12,14,16,18,20,22).
    PLAYER_DEATH_ANIM = {0, 12, 0, 12, 0, 12, 0, 12, 0, 12, 0, 12, 0, 12, 0, 12, 14, 16, 18, 20, 22, 22, 22, 22, 22, 22}
    anim_offsets = {0} | {TILE * 2 * p for p in (2, 4)} | {v * TILE for v in PLAYER_DEATH_ANIM}
    base_positions = {0, TILE * 2}  # PLAYER_SKIN1_X_L, PLAYER_SKIN1_X_R
    player_x = sorted({bx + off for bx in base_positions for off in anim_offsets})
    for x in player_x:
        entries.append(rect(f"player x={x}", x, 0, TILE * 2, TILE * 2, "Assets.cpp:playerRegion + ObjectSystem.cpp:animatePlayer"))

    # --- src/render/Hud.cpp glyph(): bitmap font cells, all TILE x TILE ---
    SYMBOLS_X, SYMBOLS_Y = 0, 48
    NUMBERS_X, NUMBERS_Y = 24, 48
    LETTERS_X, LETTERS_Y = 0, 56
    hud_y48 = set()
    for c in range(32, 34):  # symbols
        hud_y48.add(SYMBOLS_X + (c - 32) * TILE)
    for c in range(40, 42):  # '(' ')'
        hud_y48.add(NUMBERS_X + (c - 29) * TILE)
    hud_y48.add(NUMBERS_X - TILE)  # '.'
    for c in range(48, 59):  # '0'-'9' ':'
        hud_y48.add(NUMBERS_X + (c - 48) * TILE)
    for x in sorted(hud_y48):
        entries.append(rect(f"glyph@y48 x={x}", x, SYMBOLS_Y, TILE, TILE, "Hud.cpp:glyph"))
    hud_y56 = {LETTERS_X + (c - 65) * TILE for c in range(65, 91)}  # 'A'-'Z' (lowercase reuses same cells)
    for x in sorted(hud_y56):
        entries.append(rect(f"glyph@y56 x={x}", x, LETTERS_Y, TILE, TILE, "Hud.cpp:glyph"))

    return entries


def build_bosses_manifest():
    entries = []

    # --- src/systems/BossSystem.cpp showBossBody()/animateBoss(): per-stage body frames ---
    # Verified by hand-tracing every switch branch; cross-checked against blob-detection on
    # bosses.png (24 auto-detected blobs == 24 frames enumerated here).
    frames = {
        1: {"size": (40, 40), "cells": [(0, 0), (40, 0)]},
        2: {"size": (40, 40), "cells": [(80, 0)]},
        3: {"size": (40, 40), "cells": [(200, 0), (240, 0), (280, 0), (80, 40), (120, 40), (160, 40), (200, 40), (240, 40)]},
        4: {"size": (40, 48), "cells": [(0, 80)]},
        5: {"size": (40, 48), "cells": [(120, 80), (160, 80), (200, 80), (240, 80)]},
        6: {"size": (40, 48), "cells": [(0, 128), (40, 128)]},
        7: {"size": (16, 16), "cells": [(0, 56), (16, 56), (32, 56)]},
        8: {"size": (48, 48), "cells": [(80, 128), (128, 128)]},
    }
    for stage, info in frames.items():
        w, h = info["size"]
        for i, (x, y) in enumerate(info["cells"]):
            entries.append(rect(f"stage{stage} frame{i}", x, y, w, h, "BossSystem.cpp:showBossBody/animateBoss"))

    # --- src/Constants.h OBJ_TILE[50] "Boss eyes", sampled from bosses.png (see ObjectSystem.cpp
    # applyFrame/allocateObject: objId == 50 reads engine_.assets.boss(...) instead of .obj(...)) ---
    entries.append(rect("boss eye", 176, 128, 16, 16, "Constants.h:OBJ_TILE[50] (sampled from bosses.png)"))

    return entries


def dedupe_and_write(entries, sheet_name, out_path):
    by_rect = {}
    for e in entries:
        key = (e["x"], e["y"], e["w"], e["h"])
        by_rect.setdefault(key, []).append(e["name"])

    tiles = []
    for i, (key, names) in enumerate(sorted(by_rect.items(), key=lambda kv: (kv[0][1], kv[0][0]))):
        x, y, w, h = key
        tiles.append({"id": i, "x": x, "y": y, "w": w, "h": h, "labels": names})

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps({"sheet": sheet_name, "tiles": tiles}, indent=2) + "\n")
    print(f"{sheet_name}: {len(entries)} transcribed refs -> {len(tiles)} unique rects -> {out_path}")


def main():
    dedupe_and_write(build_objects_manifest(), "objects.png", OUT_DIR / "objects_manifest.json")
    dedupe_and_write(build_bosses_manifest(), "bosses.png", OUT_DIR / "bosses_manifest.json")


if __name__ == "__main__":
    main()
