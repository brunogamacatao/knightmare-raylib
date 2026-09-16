# Spritesheet restyle/upscale pipeline

Three scripts, run in order:

1. **`build_manifests.py`** -- transcribes the tile rectangles that are today
   hardcoded in the C++ source (`src/Constants.h` OBJ_TILE, `src/systems/BossSystem.cpp`,
   `src/render/Hud.cpp`) into `source_manifests/objects_manifest.json` and
   `source_manifests/bosses_manifest.json`. Re-run it if those C++ tables ever change.
   `maps.png` doesn't need a manifest: it's already a plain uniform 8x8 grid
   (`extract_atlas.py --mode grid` mirrors that formula directly).

2. **`extract_atlas.py`** -- normalizes a manifest/grid/auto-detected-blob source into
   the atlas JSON+CSV format the restyle script reads. `atlas/` holds the generated
   output for this project's three sheets:
   ```
   python3 build_manifests.py
   python3 extract_atlas.py --input ../../assets/tiles/objects.png --output atlas/objects.atlas.json --mode manifest --manifest source_manifests/objects_manifest.json
   python3 extract_atlas.py --input ../../assets/tiles/bosses.png  --output atlas/bosses.atlas.json  --mode manifest --manifest source_manifests/bosses_manifest.json
   python3 extract_atlas.py --input ../../assets/tiles/maps.png    --output atlas/maps.atlas.json    --mode grid --cell-w 8 --cell-h 8
   ```
   (There's also a `--mode blob` auto-detector, based on connected-component analysis
   of the alpha channel, useful for exploring a *new* spritesheet that has no hardcoded
   rectangles yet. It was cross-checked against the manifests above: 24/24 match on
   bosses.png, ~102/101 on objects.png.)

3. **`restyle_spritesheet.py`** -- the actual restyle+upscale+reassemble step:
   ```
   pip install -r requirements.txt
   export GEMINI_API_KEY=...   # see "Which AI" below

   python3 restyle_spritesheet.py \
     --input ../../assets/tiles/objects.png \
     --output ../../assets/tiles/objects_ghibli.png \
     --atlas atlas/objects.atlas.json \
     --scale 4 --style ghibli --provider gemini
   ```
   `--list-styles` prints the built-in presets (`disney`, `ghibli`, `watercolor`,
   `pixel-art-hd`, `dark-fantasy`, `comic`); `--style-prompt "..."` overrides with
   free text for anything else. Try `--dry-run` first (no API calls, no cost) to
   sanity-check that tiles land in the right place -- it just Lanczos-upscales
   locally instead of calling the AI.

   Each AI response is cached in `.cache/restyle/` (git-ignored) keyed by a hash of
   the crop pixels + prompt + provider + model, so an interrupted run resumes for
   free and a re-run with the same style only pays for tiles that changed.

## Which AI to use

**Recommended: Gemini "Nano Banana" image models** (`--provider gemini`, default model
`gemini-3.1-flash-image`; the earlier `gemini-2.5-flash-image` still works, pass
`--model gemini-2.5-flash-image` if you'd rather use it, but Google's docs now call it
legacy). Good at instruction-following edits that keep a reference image's
pose/silhouette/transparency intact while re-rendering it in a different art style,
which is exactly what's needed for ~100-800 small sprite crops to come out *consistent*
with each other, and cheap enough to run at that volume. Get a key at
https://aistudio.google.com/apikey and export it as `GEMINI_API_KEY`.

Under the hood this calls the newer Gemini **Interactions API**
(`POST /v1beta/interactions`, `x-goog-api-key` header), not the older
`models/*:generateContent` endpoint -- Google migrated image generation/editing there.
The REST response's exact JSON nesting for the returned image isn't fully spelled out
in the docs (only the SDK's `interaction.output_image.data` property is shown), so
`GeminiProvider` in `restyle_spritesheet.py` walks the whole response tree looking for
an `{"mime_type": "image/...", "data": "<base64>"}` pair rather than hardcoding one
path -- if Google changes the shape again, this still has a decent chance of finding it,
and raises a clear error with a response snippet if it can't.

**Alternative: OpenAI gpt-image-1** (`--provider openai`, `OPENAI_API_KEY`). Often
strong artistic quality, but its edits endpoint wants a large roughly-square canvas,
so tiny tile crops get letterboxed onto a 1024x1024 canvas and cropped back out --
works, but is both slower and pricier per tile than Gemini for this many small crops.

Plain ChatGPT/Claude chat is not a fit here: this needs a scriptable image-edit API
that takes an input image + prompt and returns a new image, not a chat UI. (Claude
itself can't generate or edit images at all.)

## The "just swap the filename" caveat

The reassembled sheet keeps every tile's position and size scaled by the exact same
factor, so proportions and spacing are preserved -- but the C++ renderer
(`src/render/Draw.h`) currently reads sprites using raw pixel rectangles sized for the
*original* 1x sheets (see `src/Constants.h`, `BossSystem.cpp`, `Hud.cpp`). Loading a
`--scale 4` sheet by only changing the filename in `Assets.cpp` will sample the wrong
(tiny top-left) corner of the new texture, not work automatically.

Making that true requires one small, separate renderer change: scale every sample
rectangle by `loadedTexture.size / originalSheet.size` at draw time. That's intentionally
not done here -- ask for it once a restyled result looks good and you're ready to wire
it into the game, since it touches `Draw.h`, `Assets.cpp` and every `.obj()/.boss()/.mapRegion()`
call site.

## Tileset seam caveat

`maps.png` tiles are restyled independently, 8x8px at a time. For sprites (characters,
items, bosses) that's fine -- each is a self-contained crop. For a *tileset* though,
adjacent tiles are meant to connect seamlessly (grass into path, etc.), and an AI
restyling each 8x8 cell in isolation has no way to keep edges consistent with its
neighbors; expect visible seams. `--padding` gives each tile a few pixels of
neighboring context but doesn't fully solve this. If seams are a problem in practice,
restyling `maps.png` as a handful of larger multi-tile crops (or as one whole-sheet
pass) instead of full auto-slicing is the next thing to try.
