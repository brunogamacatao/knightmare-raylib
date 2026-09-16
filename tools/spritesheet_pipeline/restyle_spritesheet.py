#!/usr/bin/env python3
"""Restyle + upscale a spritesheet, tile by tile, using an image-generation AI.

For every tile rectangle in an atlas (see extract_atlas.py / build_manifests.py):
  1. crop it out of the source sheet (with optional context padding),
  2. send the crop to an AI image model with a style prompt,
  3. resize the AI's result back to exactly (tile_w * scale, tile_h * scale),
  4. paste it into a new sheet at (tile_x * scale, tile_y * scale).

The output sheet has the exact same layout as the input, just uniformly
scaled -- same relative tile positions and spacing. Results and (crop, style,
provider, model) pairs are cached on disk, so an interrupted run can resume
without re-paying for tiles already done, and re-running with --dry-run costs
nothing (it skips the AI call and just does a local Lanczos upscale, useful
for sanity-checking the geometry before spending API credits for real).

Examples:
  python3 restyle_spritesheet.py \\
      --input ../../assets/tiles/objects.png \\
      --output ../../assets/tiles/objects_ghibli.png \\
      --atlas atlas/objects.atlas.json \\
      --scale 4 --style ghibli --provider gemini

  # Try the pipeline for free first (no API calls, just upscales locally):
  python3 restyle_spritesheet.py --input ... --output ... --atlas ... --scale 4 --style ghibli --dry-run

IMPORTANT CAVEAT (read before wiring the result into the game):
The C++ renderer (src/render/Draw.h) currently samples sprites using raw,
un-normalized pixel rectangles from src/Constants.h / BossSystem.cpp /
Hud.cpp, sized for the ORIGINAL (1x) sheets. Dropping in a --scale 4 sheet by
filename alone will NOT work correctly until the renderer is changed to
multiply every sample rectangle by (loaded_texture_size / original_sheet_size).
That renderer change is a separate, small piece of work -- ask for it
explicitly once you're happy with a restyled result and want to wire it in.
"""
from __future__ import annotations

import argparse
import base64
import hashlib
import io
import json
import os
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

import requests
from PIL import Image

# --------------------------------------------------------------------------- styles

STYLE_PRESETS = {
    "disney": (
        "Classic Disney Renaissance animated-film art style: warm, painterly shading, "
        "expressive rounded shapes, clean bold outlines, rich saturated colors, soft "
        "cel-shaded lighting."
    ),
    "ghibli": (
        "Studio Ghibli hand-painted style: soft watercolor-influenced shading, gentle "
        "muted color palette, warm lighting, painterly organic textures, whimsical and "
        "cozy character design."
    ),
    "watercolor": (
        "Traditional watercolor illustration style: visible paper texture, soft bleeding "
        "edges, translucent color washes, delicate linework, gentle desaturated palette."
    ),
    "pixel-art-hd": (
        "Polished high-resolution pixel art style (HD-2D / modern SNES-revival look): crisp "
        "clean pixel clusters, strong readable silhouette, vibrant colors, subtle dithered "
        "shading, no anti-aliasing blur."
    ),
    "dark-fantasy": (
        "Dark fantasy game concept-art style: dramatic moody lighting, textured painterly "
        "rendering, desaturated palette with selective strong accent colors, gritty detail."
    ),
    "comic": (
        "Western comic-book style: bold black ink outlines, flat cel shading, halftone-style "
        "accents, punchy saturated colors, dynamic graphic silhouette."
    ),
}

PROMPT_TEMPLATE = """You are restyling ONE sprite cut from a 2D video game spritesheet.

Target art style: {style}

Hard rules, follow exactly:
- Redraw this exact sprite in the target style. Keep the same pose, silhouette, facing
  direction, proportions and framing -- do not add, remove, crop, rotate or mirror content.
- The background MUST stay fully transparent (alpha = 0). Output a PNG with an alpha channel.
  Do not add a background color, ground shadow, border, frame or watermark.
- Do not add extra characters, props, or scenery that are not present in the input.
- This is a single reusable game asset, not a scene or illustration composition.
- Keep the output centered and filling the same relative frame as the input crop.
"""


def build_prompt(style_key: str, style_prompt: str | None) -> str:
    style_text = style_prompt or STYLE_PRESETS.get(style_key)
    if style_text is None:
        raise SystemExit(f"error: unknown --style '{style_key}' (use --list-styles, or pass --style-prompt)")
    return PROMPT_TEMPLATE.format(style=style_text)


# --------------------------------------------------------------------------- providers

class ProviderError(RuntimeError):
    pass


def _find_image_data(node) -> str | None:
    """Recursively search a decoded JSON response for an {"mime_type": "image/...", "data": "<b64>"}
    pair. The Gemini Interactions API's exact REST response nesting isn't fully documented (the docs
    only show the SDK property `interaction.output_image.data`), so rather than hardcode one path and
    break silently if it's nested differently, walk the whole tree and grab the first image blob."""
    if isinstance(node, dict):
        mime = node.get("mime_type") or node.get("mimeType")
        data = node.get("data")
        if isinstance(mime, str) and mime.startswith("image/") and isinstance(data, str):
            return data
        for v in node.values():
            found = _find_image_data(v)
            if found:
                return found
    elif isinstance(node, list):
        for v in node:
            found = _find_image_data(v)
            if found:
                return found
    return None


class GeminiProvider:
    """Gemini Nano Banana image model, via the Interactions REST API.

    gemini-3.1-flash-image is the current general-purpose "Nano Banana" model (the earlier
    gemini-2.5-flash-image is documented as legacy but still usable via --model if you prefer it).
    """

    def __init__(self, api_key: str, model: str = "gemini-3.1-flash-image"):
        self.api_key = api_key
        self.model = model

    def restyle(self, png_bytes: bytes, prompt: str) -> bytes:
        url = "https://generativelanguage.googleapis.com/v1beta/interactions"
        body = {
            "model": self.model,
            "input": [
                {"type": "text", "text": prompt},
                {"type": "image", "mime_type": "image/png", "data": base64.b64encode(png_bytes).decode()},
            ],
        }
        resp = requests.post(url, headers={"x-goog-api-key": self.api_key, "Content-Type": "application/json"},
                              json=body, timeout=120)
        if resp.status_code != 200:
            raise ProviderError(f"gemini http {resp.status_code}: {resp.text[:500]}")
        data = resp.json()
        image_b64 = _find_image_data(data)
        if not image_b64:
            raise ProviderError(f"gemini: no image data found in response: {json.dumps(data)[:500]}")
        return base64.b64decode(image_b64)


class OpenAIProvider:
    """OpenAI gpt-image-1, via the images/edits REST API.

    The edits endpoint wants a reasonably large, roughly-square canvas, which our
    tiny tile crops are not -- so we letterbox the crop onto a transparent square
    canvas before sending, and crop the corresponding region back out afterwards.
    """

    def __init__(self, api_key: str, model: str = "gpt-image-1", canvas: int = 1024):
        self.api_key = api_key
        self.model = model
        self.canvas = canvas

    def restyle(self, png_bytes: bytes, prompt: str) -> bytes:
        src = Image.open(io.BytesIO(png_bytes)).convert("RGBA")
        w, h = src.size
        scale = min(self.canvas / w, self.canvas / h)
        inner_w, inner_h = max(1, round(w * scale)), max(1, round(h * scale))
        canvas = Image.new("RGBA", (self.canvas, self.canvas), (0, 0, 0, 0))
        ox, oy = (self.canvas - inner_w) // 2, (self.canvas - inner_h) // 2
        canvas.paste(src.resize((inner_w, inner_h), Image.LANCZOS), (ox, oy))
        buf = io.BytesIO()
        canvas.save(buf, format="PNG")

        resp = requests.post(
            "https://api.openai.com/v1/images/edits",
            headers={"Authorization": f"Bearer {self.api_key}"},
            files={"image": ("tile.png", buf.getvalue(), "image/png")},
            data={"model": self.model, "prompt": prompt, "size": f"{self.canvas}x{self.canvas}",
                  "background": "transparent", "n": 1},
            timeout=180,
        )
        if resp.status_code != 200:
            raise ProviderError(f"openai http {resp.status_code}: {resp.text[:500]}")
        data = resp.json()
        try:
            b64 = data["data"][0]["b64_json"]
        except (KeyError, IndexError) as e:
            raise ProviderError(f"openai: unexpected response shape: {json.dumps(data)[:500]}") from e
        out = Image.open(io.BytesIO(base64.b64decode(b64))).convert("RGBA")
        out = out.resize((self.canvas, self.canvas), Image.LANCZOS) if out.size != (self.canvas, self.canvas) else out
        cropped = out.crop((ox, oy, ox + inner_w, oy + inner_h))
        buf2 = io.BytesIO()
        cropped.save(buf2, format="PNG")
        return buf2.getvalue()


def make_provider(name: str, api_key: str | None, model: str | None):
    if name == "gemini":
        key = api_key or os.environ.get("GEMINI_API_KEY")
        if not key:
            raise SystemExit("error: set GEMINI_API_KEY or pass --api-key")
        return GeminiProvider(key, model or "gemini-3.1-flash-image")
    if name == "openai":
        key = api_key or os.environ.get("OPENAI_API_KEY")
        if not key:
            raise SystemExit("error: set OPENAI_API_KEY or pass --api-key")
        return OpenAIProvider(key, model or "gpt-image-1")
    raise SystemExit(f"error: unknown provider '{name}'")


# --------------------------------------------------------------------------- pipeline

def cache_key(png_bytes: bytes, prompt: str, provider: str, model: str) -> str:
    h = hashlib.sha256()
    h.update(png_bytes)
    h.update(prompt.encode())
    h.update(provider.encode())
    h.update(model.encode())
    return h.hexdigest()


def crop_tile(sheet: Image.Image, tile: dict, padding: int) -> tuple[bytes, tuple[int, int, int, int]]:
    x0 = max(0, tile["x"] - padding)
    y0 = max(0, tile["y"] - padding)
    x1 = min(sheet.width, tile["x"] + tile["w"] + padding)
    y1 = min(sheet.height, tile["y"] + tile["h"] + padding)
    crop = sheet.crop((x0, y0, x1, y1))
    buf = io.BytesIO()
    crop.save(buf, format="PNG")
    # offset of the *real* tile within this padded crop
    core = (tile["x"] - x0, tile["y"] - y0, tile["w"], tile["h"])
    return buf.getvalue(), core


def restyle_one_tile(tile, sheet_img, provider, prompt, padding, scale, min_ai_size, cache_dir, dry_run):
    png_bytes, (cx, cy, cw, ch) = crop_tile(sheet_img, tile, padding)
    target_w, target_h = max(1, round(tile["w"] * scale)), max(1, round(tile["h"] * scale))

    if dry_run or min(tile["w"], tile["h"]) < min_ai_size:
        core = Image.open(io.BytesIO(png_bytes)).convert("RGBA").crop((cx, cy, cx + cw, cy + ch))
        result = core.resize((target_w, target_h), Image.LANCZOS)
        return tile["id"], result

    key = cache_key(png_bytes, prompt, provider.__class__.__name__, getattr(provider, "model", ""))
    cache_path = (cache_dir / f"{key}.png") if cache_dir else None
    if cache_path and cache_path.exists():
        out_bytes = cache_path.read_bytes()
    else:
        last_err = None
        out_bytes = None
        for attempt in range(4):
            try:
                out_bytes = provider.restyle(png_bytes, prompt)
                break
            except ProviderError as e:
                last_err = e
                time.sleep(2 ** attempt)
        if out_bytes is None:
            raise last_err
        if cache_path:
            cache_dir.mkdir(parents=True, exist_ok=True)
            cache_path.write_bytes(out_bytes)

    out_img = Image.open(io.BytesIO(out_bytes)).convert("RGBA")
    # The AI reply is arbitrary-resolution; resize it back to the padded crop's pixel
    # size so the core/padding offsets computed above line back up, then lift the core.
    padded_size = Image.open(io.BytesIO(png_bytes)).size
    if out_img.size != padded_size:
        out_img = out_img.resize(padded_size, Image.LANCZOS)
    core = out_img.crop((cx, cy, cx + cw, cy + ch))
    result = core.resize((target_w, target_h), Image.LANCZOS)
    return tile["id"], result


def check_overlaps(tiles, scale):
    boxes = [(t["id"], t["x"] * scale, t["y"] * scale, (t["x"] + t["w"]) * scale, (t["y"] + t["h"]) * scale) for t in tiles]
    for i in range(len(boxes)):
        for j in range(i + 1, len(boxes)):
            _, ax0, ay0, ax1, ay1 = boxes[i]
            _, bx0, by0, bx1, by1 = boxes[j]
            if ax0 < bx1 and bx0 < ax1 and ay0 < by1 and by0 < ay1:
                print(f"warning: scaled tiles {boxes[i][0]} and {boxes[j][0]} overlap in the output canvas", file=sys.stderr)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--input", required=True, type=Path)
    ap.add_argument("--output", required=True, type=Path)
    ap.add_argument("--atlas", type=Path, help="Atlas JSON from extract_atlas.py (required unless --list-styles)")
    ap.add_argument("--scale", type=float, default=1.0, help="Upscale factor applied uniformly to every tile and its position")
    ap.add_argument("--style", default="disney", help=f"One of: {', '.join(STYLE_PRESETS)} (or anything, combined with --style-prompt)")
    ap.add_argument("--style-prompt", help="Freeform style description, overrides --style preset text")
    ap.add_argument("--provider", choices=["gemini", "openai"], default="gemini")
    ap.add_argument("--model", help="Override the provider's default model id")
    ap.add_argument("--api-key", help="Overrides GEMINI_API_KEY / OPENAI_API_KEY env vars")
    ap.add_argument("--padding", type=int, default=2, help="Context pixels included around each crop before sending to the AI")
    ap.add_argument("--min-ai-size", type=int, default=6, help="Tiles smaller than this (px, min side) are Lanczos-upscaled locally instead of sent to the AI")
    ap.add_argument("--concurrency", type=int, default=4)
    ap.add_argument("--cache-dir", type=Path, default=Path(".cache/restyle"))
    ap.add_argument("--dry-run", action="store_true", help="Skip AI calls entirely; local Lanczos upscale only (free -- use to sanity-check geometry)")
    ap.add_argument("--limit", type=int, help="Only process the first N tiles (debugging)")
    ap.add_argument("--list-styles", action="store_true")
    args = ap.parse_args()

    if args.list_styles:
        for name, desc in STYLE_PRESETS.items():
            print(f"{name}: {desc}")
        return

    if not args.atlas:
        sys.exit("error: --atlas is required")
    if not args.input.exists():
        sys.exit(f"error: input not found: {args.input}")

    atlas = json.loads(args.atlas.read_text())
    sheet_img = Image.open(args.input).convert("RGBA")
    if atlas["width"] != sheet_img.width or atlas["height"] != sheet_img.height:
        sys.exit(f"error: atlas was built for a {atlas['width']}x{atlas['height']} sheet, "
                  f"but --input is {sheet_img.width}x{sheet_img.height}")

    tiles = atlas["tiles"]
    if args.limit:
        tiles = tiles[:args.limit]

    check_overlaps(tiles, args.scale)

    prompt = build_prompt(args.style, args.style_prompt)
    provider = None if args.dry_run else make_provider(args.provider, args.api_key, args.model)
    cache_dir = None if args.dry_run else args.cache_dir

    out_w = max(1, round(sheet_img.width * args.scale))
    out_h = max(1, round(sheet_img.height * args.scale))
    out_canvas = Image.new("RGBA", (out_w, out_h), (0, 0, 0, 0))

    print(f"{args.input.name}: {len(tiles)} tiles, scale={args.scale}, style={args.style!r}, "
          f"provider={'(dry-run)' if args.dry_run else args.provider}")
    print(f"output canvas: {out_w}x{out_h}")

    done = 0
    with ThreadPoolExecutor(max_workers=args.concurrency) as pool:
        futures = {
            pool.submit(restyle_one_tile, tile, sheet_img, provider, prompt, args.padding,
                        args.scale, args.min_ai_size, cache_dir, args.dry_run): tile
            for tile in tiles
        }
        for future in as_completed(futures):
            tile = futures[future]
            try:
                tile_id, result_img = future.result()
            except Exception as e:  # noqa: BLE001 - surface any provider/network failure per tile
                print(f"FAILED tile {tile['id']} {tile.get('labels', '')}: {e}", file=sys.stderr)
                continue
            px, py = round(tile["x"] * args.scale), round(tile["y"] * args.scale)
            out_canvas.alpha_composite(result_img, (px, py))
            done += 1
            print(f"  [{done}/{len(tiles)}] tile {tile_id} {tile.get('labels', '')} -> ({px},{py}) {result_img.size}")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    out_canvas.save(args.output)
    print(f"wrote {args.output} ({done}/{len(tiles)} tiles restyled)")


if __name__ == "__main__":
    main()
