#!/usr/bin/env python3
"""Draws Gunk's HeartGold DexNav screens and converts them into DS tile data.

The layout follows Gunk's Platinum DexNav (info and search cards on the top screen, LAND and WATER grids on the
touch screen); the look is HeartGold's: the green striped touch-screen background, white bevelled panels, a gold
title bar, and the red outline HGSS puts on the touched button. Each 256x192 screen is cut into 8x8 tiles,
deduplicated (with H/V flips) and written to src/gunk_dexnav_art.h as 4bpp tiles, tilemaps and palettes. The
cursor is a palette swap, so selected cells cost no tiles.

Usage: python dexnav_art.py <repo root> [preview dir]
"""
import os
import sys

from PIL import Image, ImageDraw

W, H = 256, 192

# Layout shared with src/gunk_dexnav_app.c
GRID_LEFT, CELL_W, CELL_H, COLS, ROWS = 8, 40, 32, 6, 2
LAND_TOP, WATER_TOP = 16, 104
FOOTER_TOP = 176
CLOSE_LEFT, CLOSE_RIGHT = 192, 251

# Palette 0: base art. Index 0 is never drawn (BG transparency).
BASE = [
    (0, 0, 0),         # 0  transparent
    (48, 112, 72),     # 1  background green
    (64, 136, 88),     # 2  background stripe
    (24, 64, 40),      # 3  title bar dark
    (232, 184, 56),    # 4  gold
    (160, 112, 24),    # 5  gold shadow
    (248, 248, 240),   # 6  panel
    (104, 128, 112),   # 7  panel border
    (224, 236, 228),   # 8  slot fill
    (168, 192, 176),   # 9  slot shadow
    (96, 128, 112),    # 10 slot border
    (208, 220, 212),   # 11 panel shade
    (48, 48, 48),      # 12 ink
    (224, 64, 48),     # 13 red
    (200, 232, 208),   # 14 tab
    (255, 255, 255),   # 15 shine
]
# Palette 1: the selected cell: warm fill and HGSS's red outline.
SELECTED = list(BASE)
SELECTED[8] = (255, 236, 200)
SELECTED[9] = (240, 160, 80)
SELECTED[10] = (224, 64, 48)
SELECTED[15] = (255, 248, 232)

# Palette 2: text (font colour slots).
TEXT = [(0, 0, 0)] * 16
TEXT[1] = (48, 48, 48)      # ink
TEXT[2] = (192, 200, 192)   # shadow on panels
TEXT[3] = (255, 255, 255)   # light text on the title bar
TEXT[4] = (24, 64, 40)      # its shadow
TEXT[5] = (224, 64, 48)     # red
TEXT[6] = (248, 192, 176)   # its shadow
TEXT[15] = (248, 248, 240)


def new_screen():
    img = Image.new("P", (W, H), 1)
    img.putpalette([c for rgb in BASE for c in rgb] + [0] * (256 - 16) * 3)
    d = ImageDraw.Draw(img)
    # HGSS's diagonal stripes, 2 px wide every 8
    for y in range(H):
        for x in range(W):
            if (x + y) % 8 < 2:
                d.point((x, y), fill=2)
    return img, d


def bevel_panel(d, x0, y0, x1, y1, fill=6, r=4):
    # drop shadow, border, fill, then a light top edge and a shaded bottom edge
    d.rounded_rectangle([x0 + 1, y0 + 2, x1 + 1, y1 + 2], radius=r, fill=3)
    d.rounded_rectangle([x0, y0, x1, y1], radius=r, fill=fill, outline=7)
    d.line([x0 + r, y0 + 1, x1 - r, y0 + 1], fill=15)
    d.line([x0 + r, y1 - 1, x1 - r, y1 - 1], fill=11)


def slot(d, x, y):
    # cell is 40x32; the button sits inside with a shadow under it
    d.rounded_rectangle([x + 3, y + 3, x + 37, y + 30], radius=4, fill=9)
    d.rounded_rectangle([x + 2, y + 1, x + 36, y + 28], radius=4, fill=8, outline=10)
    d.line([x + 6, y + 2, x + 30, y + 2], fill=15)


def ball(d, x, y):
    # a tiny Poké Ball: the section marks
    d.ellipse([x, y, x + 6, y + 6], fill=15, outline=12)
    d.rectangle([x + 1, y + 1, x + 5, y + 2], fill=13)
    d.line([x, y + 3, x + 6, y + 3], fill=12)
    d.point((x + 3, y + 3), fill=15)


def section(d, top, label_w):
    bevel_panel(d, 3, top - 13, 252, top + ROWS * CELL_H + 3)
    d.rounded_rectangle([6, top - 12, 6 + label_w, top - 2], radius=3, fill=14, outline=7)
    ball(d, 6 + label_w - 9, top - 10)
    for i in range(COLS * ROWS):
        slot(d, GRID_LEFT + (i % COLS) * CELL_W, top + (i // COLS) * CELL_H)


def title_bar(d, y0, y1):
    d.rectangle([0, y0, 255, y1], fill=3)
    d.line([0, y1 - 1, 255, y1 - 1], fill=5)
    d.line([0, y1, 255, y1], fill=4)


def draw_bottom():
    img, d = new_screen()
    section(d, LAND_TOP, 46)
    section(d, WATER_TOP, 52)
    title_bar(d, FOOTER_TOP, 191)
    d.line([0, FOOTER_TOP, 255, FOOTER_TOP], fill=4)
    d.line([0, 191, 255, 191], fill=3)
    # the Close button, outlined in red like HGSS's touch buttons
    d.rounded_rectangle([CLOSE_LEFT, FOOTER_TOP + 2, CLOSE_RIGHT, 189], radius=3, fill=6, outline=13)
    d.line([CLOSE_LEFT + 3, FOOTER_TOP + 3, CLOSE_RIGHT - 3, FOOTER_TOP + 3], fill=15)
    return img


def draw_top():
    img, d = new_screen()
    title_bar(d, 0, 21)
    ball(d, 60, 7)
    # info card
    bevel_panel(d, 8, 30, 247, 124, r=6)
    d.line([16, 58, 239, 58], fill=7)
    for yy in (82, 104):
        for xx in range(16, 240, 4):
            d.point((xx, yy), fill=9)
    ball(d, 228, 36)
    # search card
    bevel_panel(d, 8, 132, 247, 186, r=6)
    d.rounded_rectangle([12, 136, 64, 146], radius=3, fill=14, outline=7)
    ball(d, 54, 138)
    return img


def to_tiles(img):
    px = img.load()
    tiles = []
    for ty in range(H // 8):
        for tx in range(W // 8):
            tiles.append(tuple(px[tx * 8 + x, ty * 8 + y] for y in range(8) for x in range(8)))
    return tiles


def flips(t):
    rows = [t[i * 8:(i + 1) * 8] for i in range(8)]
    h = tuple(v for r in rows for v in reversed(r))
    v = tuple(v for r in reversed(rows) for v in r)
    hv = tuple(v for r in reversed(rows) for v in reversed(r))
    return [(t, 0), (h, 1), (v, 2), (hv, 3)]


def build(images):
    tileset, index, maps = [], {}, []
    for img in images:
        m = []
        for t in to_tiles(img):
            for cand, flip in flips(t):
                if cand in index:
                    m.append(index[cand] | (flip << 10))
                    break
            else:
                index[t] = len(tileset)
                tileset.append(t)
                m.append(index[t])
        maps.append(m)
    return tileset, maps


def pack(t):
    words = []
    for i in range(0, 64, 8):
        w = 0
        for j in range(8):
            w |= (t[i + j] & 0xF) << (4 * j)
        words.append(w)
    return words


def rgb555(c):
    r, g, b = (v >> 3 for v in c)
    return r | (g << 5) | (b << 10)


def c_array(ctype, name, values, per_line=8, fmt="0x{:04X}"):
    lines = [f"static const {ctype} {name}[{len(values)}] = {{"]
    for i in range(0, len(values), per_line):
        lines.append("    " + ", ".join(fmt.format(v) for v in values[i:i + per_line]) + ",")
    lines.append("};")
    return "\n".join(lines)


def main():
    root = sys.argv[1]
    preview = sys.argv[2] if len(sys.argv) > 2 else None
    top, bottom = draw_top(), draw_bottom()
    tileset, (topMap, bottomMap) = build([top, bottom])

    out = [
        "// Generated by tools/gunk/dexnav_art.py - do not edit by hand.",
        "#ifndef POKEHEARTGOLD_GUNK_DEXNAV_ART_H",
        "#define POKEHEARTGOLD_GUNK_DEXNAV_ART_H",
        "",
        f"#define DEXNAV_ART_NUM_TILES {len(tileset)}",
        "",
        c_array("u16", "sDexNavPalBase", [rgb555(c) for c in BASE]),
        c_array("u16", "sDexNavPalSelected", [rgb555(c) for c in SELECTED]),
        c_array("u16", "sDexNavPalText", [rgb555(c) for c in TEXT]),
        c_array("u32", "sDexNavTiles", [w for t in tileset for w in pack(t)], fmt="0x{:08X}"),
        c_array("u16", "sDexNavTopMap", topMap, per_line=16),
        c_array("u16", "sDexNavBottomMap", bottomMap, per_line=16),
        "",
        "#endif // POKEHEARTGOLD_GUNK_DEXNAV_ART_H",
        "",
    ]
    with open(os.path.join(root, "src/gunk_dexnav_art.h"), "w", newline="\n") as f:
        f.write("\n".join(out))

    if preview:
        os.makedirs(preview, exist_ok=True)
        top.convert("RGB").resize((W * 2, H * 2), Image.NEAREST).save(os.path.join(preview, "dexnav_top.png"))
        bottom.convert("RGB").resize((W * 2, H * 2), Image.NEAREST).save(os.path.join(preview, "dexnav_bottom.png"))

    print(f"{len(tileset)} unique tiles ({len(tileset) * 32} bytes) for 1536 screen tiles")


if __name__ == "__main__":
    main()
