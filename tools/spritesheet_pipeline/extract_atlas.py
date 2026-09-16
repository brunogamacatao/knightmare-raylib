#!/usr/bin/env python3
"""Detect sprite/tile regions inside a spritesheet PNG and write them to JSON/CSV.

Two detection modes:
  - blob: connected-component detection on the alpha channel. Good for loosely
    packed sheets where sprites are separated by transparent padding
    (objects.png, bosses.png in this project).
  - grid: fixed-size cell slicing. Good for tightly packed tilesets where
    tiles touch each other with no gap (maps.png in this project).

The output atlas JSON is the contract the restyle_spritesheet.py script reads
to know where to cut each tile out of the source sheet, and where to paste it
back (scaled) into the restyled sheet. It is intentionally independent of any
game source code -- it is derived straight from the pixels of the PNG.

Usage:
  python3 extract_atlas.py --input ../../assets/tiles/objects.png \
      --output atlas/objects.atlas.json --mode blob

  python3 extract_atlas.py --input ../../assets/tiles/maps.png \
      --output atlas/maps.atlas.json --mode grid --cell-w 8 --cell-h 8
"""
import argparse
import csv
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage


def load_rgba(path: Path) -> np.ndarray:
    img = Image.open(path).convert("RGBA")
    return np.array(img)


def detect_blobs(rgba: np.ndarray, alpha_threshold: int, merge: int, min_area: int):
    alpha = rgba[:, :, 3]
    mask = alpha > alpha_threshold

    if merge > 0:
        struct = np.ones((merge * 2 + 1, merge * 2 + 1), dtype=bool)
        dilated = ndimage.binary_dilation(mask, structure=struct)
    else:
        dilated = mask

    labels, count = ndimage.label(dilated, structure=np.ones((3, 3), dtype=int))
    boxes = ndimage.find_objects(labels)

    tiles = []
    for label_id, box in enumerate(boxes, start=1):
        if box is None:
            continue
        y_slice, x_slice = box
        region_mask = mask[y_slice, x_slice] & (labels[y_slice, x_slice] == label_id)
        if not region_mask.any():
            continue
        ys, xs = np.where(region_mask)
        x0, x1 = x_slice.start + xs.min(), x_slice.start + xs.max()
        y0, y1 = y_slice.start + ys.min(), y_slice.start + ys.max()
        w, h = int(x1 - x0 + 1), int(y1 - y0 + 1)
        if w * h < min_area:
            continue
        tiles.append({"x": int(x0), "y": int(y0), "w": w, "h": h})

    tiles.sort(key=lambda t: (t["y"], t["x"]))
    for i, t in enumerate(tiles):
        t["id"] = i
    tiles.sort(key=lambda t: t["id"])
    return tiles


def load_manifest(manifest_path: Path, width: int, height: int):
    data = json.loads(manifest_path.read_text())
    tiles = []
    for t in data["tiles"]:
        x, y, w, h = t["x"], t["y"], t["w"], t["h"]
        if x < 0 or y < 0 or x + w > width or y + h > height:
            print(f"warning: tile id={t['id']} ({x},{y},{w}x{h}) exceeds sheet bounds {width}x{height}", file=sys.stderr)
        tile = {"id": t["id"], "x": x, "y": y, "w": w, "h": h}
        if "labels" in t:
            tile["labels"] = t["labels"]
        tiles.append(tile)
    return tiles


def detect_grid(rgba: np.ndarray, cell_w: int, cell_h: int, cols: int | None, rows: int | None):
    height, width = rgba.shape[0], rgba.shape[1]
    cols = cols or (width // cell_w)
    rows = rows or (height // cell_h)
    tiles = []
    tile_id = 0
    for row in range(rows):
        for col in range(cols):
            tiles.append({
                "id": tile_id,
                "x": col * cell_w,
                "y": row * cell_h,
                "w": cell_w,
                "h": cell_h,
                "col": col,
                "row": row,
            })
            tile_id += 1
    return tiles, cols, rows


def write_outputs(atlas: dict, output_json: Path, write_csv: bool):
    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_json.write_text(json.dumps(atlas, indent=2) + "\n")

    if write_csv:
        csv_path = output_json.with_suffix(".csv")
        fieldnames = sorted({k for t in atlas["tiles"] for k in t.keys()}, key=lambda k: (k != "id", k))
        with csv_path.open("w", newline="") as f:
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()
            for t in atlas["tiles"]:
                row = {k: ("|".join(v) if isinstance(v, list) else v) for k, v in t.items()}
                writer.writerow(row)
        return csv_path
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--input", required=True, type=Path, help="Source spritesheet PNG")
    ap.add_argument("--output", required=True, type=Path, help="Output atlas JSON path (a sibling .csv is written too)")
    ap.add_argument("--mode", choices=["blob", "grid", "manifest"], default="blob")
    ap.add_argument("--manifest", type=Path, help="Manifest JSON path (manifest mode; see build_manifests.py)")

    # blob mode
    ap.add_argument("--alpha-threshold", type=int, default=10, help="Alpha value above which a pixel counts as opaque (blob mode)")
    ap.add_argument("--merge", type=int, default=0, help="Dilate mask by N px before labeling, to bridge anti-aliasing gaps within one sprite (blob mode)")
    ap.add_argument("--min-area", type=int, default=4, help="Drop detected blobs smaller than this pixel area (blob mode)")

    # grid mode
    ap.add_argument("--cell-w", type=int, default=8)
    ap.add_argument("--cell-h", type=int, default=8)
    ap.add_argument("--cols", type=int, default=None)
    ap.add_argument("--rows", type=int, default=None)

    ap.add_argument("--no-csv", action="store_true")
    args = ap.parse_args()

    if not args.input.exists():
        sys.exit(f"error: input not found: {args.input}")

    rgba = load_rgba(args.input)
    height, width = rgba.shape[0], rgba.shape[1]

    if args.mode == "blob":
        tiles = detect_blobs(rgba, args.alpha_threshold, args.merge, args.min_area)
        atlas = {
            "sheet": args.input.name,
            "width": int(width),
            "height": int(height),
            "mode": "blob",
            "tiles": tiles,
        }
    elif args.mode == "manifest":
        if not args.manifest:
            sys.exit("error: --manifest is required in manifest mode")
        tiles = load_manifest(args.manifest, width, height)
        atlas = {
            "sheet": args.input.name,
            "width": int(width),
            "height": int(height),
            "mode": "manifest",
            "manifest": str(args.manifest),
            "tiles": tiles,
        }
    else:
        tiles, cols, rows = detect_grid(rgba, args.cell_w, args.cell_h, args.cols, args.rows)
        atlas = {
            "sheet": args.input.name,
            "width": int(width),
            "height": int(height),
            "mode": "grid",
            "cell_w": args.cell_w,
            "cell_h": args.cell_h,
            "cols": cols,
            "rows": rows,
            "tiles": tiles,
        }

    csv_path = write_outputs(atlas, args.output, not args.no_csv)

    print(f"{args.input.name}: {width}x{height}px, mode={args.mode}, {len(atlas['tiles'])} tiles")
    print(f"  -> {args.output}")
    if csv_path:
        print(f"  -> {csv_path}")


if __name__ == "__main__":
    main()
