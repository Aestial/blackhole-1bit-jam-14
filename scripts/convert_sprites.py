#!/usr/bin/env python3
"""
convert_sprites.py — Converts PNG spritesheets into Arduboy PROGMEM C arrays.

Usage:
    python3 scripts/convert_sprites.py

This script reads images from the assets/ directory:
  - player_static.png (16x16)
  - whitehole_32.png  (128x32 -> 4 frames of 32x32)
  - items_16.png      (144x32 -> 18 frames of 16x16)

And generates assets/sprites.h formatted for Arduboy2 Sprites::drawSelfMasked
and Sprites::drawOverwrite.
"""

import os
import sys
from PIL import Image

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(SCRIPT_DIR)
ASSETS_DIR = os.path.join(ROOT_DIR, "assets")
OUTPUT_HEADER = os.path.join(ASSETS_DIR, "sprites.h")


def image_to_arduboy_bytes(img, frame_w, frame_h):
    """
    Converts a PIL Image into Arduboy vertical page format:
      Byte 0: width
      Byte 1: height
      Followed by (height / 8) * width bytes per frame.
      Bit 0 is the topmost pixel in each 8-pixel column.
    """
    img = img.convert("RGBA")
    total_w, total_h = img.size
    cols = total_w // frame_w
    rows = total_h // frame_h
    total_frames = cols * rows

    bytes_out = [frame_w, frame_h]

    for r in range(rows):
        for c in range(cols):
            ox = c * frame_w
            oy = r * frame_h
            for page in range(frame_h // 8):
                for x in range(frame_w):
                    byte_val = 0
                    for bit in range(8):
                        y = page * 8 + bit
                        px, py = ox + x, oy + y
                        red, green, blue, alpha = img.getpixel((px, py))
                        brightness = (red + green + blue) // 3
                        # White pixel if opaque and bright
                        if alpha >= 128 and brightness > 128:
                            byte_val |= (1 << bit)
                    bytes_out.append(byte_val)

    return total_frames, bytes_out


def format_c_array(name, byte_list, bytes_per_line=16):
    lines = []
    lines.append(f"const uint8_t {name}[] PROGMEM = {{")
    lines.append(f"  {byte_list[0]}, {byte_list[1]}, // Width: {byte_list[0]}, Height: {byte_list[1]}")

    data = byte_list[2:]
    for i in range(0, len(data), bytes_per_line):
        chunk = data[i:i + bytes_per_line]
        hex_strs = [f"0x{b:02x}" for b in chunk]
        lines.append("  " + ", ".join(hex_strs) + ",")

    lines.append("};")
    return "\n".join(lines)


def main():
    print(f"Scanning {ASSETS_DIR} for sprite assets...")

    player_png = os.path.join(ASSETS_DIR, "player_static.png")
    whitehole_png = os.path.join(ASSETS_DIR, "whitehole_32.png")
    items_png = os.path.join(ASSETS_DIR, "items_16.png")

    title_png = os.path.join(ASSETS_DIR, "title_screen.png")

    if not all(os.path.isfile(p) for p in [player_png, whitehole_png, items_png]):
        print("Error: Missing expected PNG files in assets/", file=sys.stderr)
        sys.exit(1)

    # 1. Player
    player_img = Image.open(player_png)
    p_frames, p_bytes = image_to_arduboy_bytes(player_img, 16, 16)
    print(f"  Player: {p_frames} frame(s), {len(p_bytes)} bytes")

    # 2. Whitehole
    wh_img = Image.open(whitehole_png)
    wh_frames, wh_bytes = image_to_arduboy_bytes(wh_img, 32, 32)
    print(f"  Whitehole: {wh_frames} frames, {len(wh_bytes)} bytes")

    # 3. Items
    items_img = Image.open(items_png)
    it_frames, it_bytes = image_to_arduboy_bytes(items_img, 16, 16)
    print(f"  Items: {it_frames} items (16x16), {len(it_bytes)} bytes")

    # 4. Title Screen (if present)
    has_title = os.path.isfile(title_png)
    t_bytes = []
    if has_title:
        title_img = Image.open(title_png).convert("RGBA")
        # Blank out the static whitehole (x=92..127, y=30..63) so the animated
        # whitehole_sprite can be drawn dynamically over the background.
        for y in range(30, 64):
            for x in range(92, 128):
                title_img.putpixel((x, y), (0, 0, 0, 255))
        t_frames, t_bytes = image_to_arduboy_bytes(title_img, 128, 64)
        print(f"  Title Screen (whitehole blanked for animation): {t_frames} frame(s), {len(t_bytes)} bytes")

    # 5. Title Font (custom bold pixel font matching title artwork)
    FONT_GLYPHS = {}
    # Uppercase
    FONT_GLYPHS["A"] = [".#####.", "##...##", "##...##", "#######", "##...##", "##...##", "##...##", "......."]
    FONT_GLYPHS["B"] = ["######.", "##...##", "##...##", "######.", "##...##", "##...##", "######.", "......."]
    FONT_GLYPHS["C"] = [".#####.", "##...##", "##.....", "##.....", "##.....", "##...##", ".#####.", "......."]
    FONT_GLYPHS["D"] = ["######.", "##...##", "##...##", "##...##", "##...##", "##...##", "######.", "......."]
    FONT_GLYPHS["E"] = ["#######", "##.....", "##.....", "######.", "##.....", "##.....", "#######", "......."]
    FONT_GLYPHS["F"] = ["#######", "##.....", "##.....", "######.", "##.....", "##.....", "##.....", "......."]
    FONT_GLYPHS["G"] = [".#####.", "##...##", "##.....", "##.####", "##...##", "##...##", ".#####.", "......."]
    FONT_GLYPHS["H"] = ["##...##", "##...##", "##...##", "#######", "##...##", "##...##", "##...##", "......."]
    FONT_GLYPHS["I"] = ["######.", "..##...", "..##...", "..##...", "..##...", "..##...", "######.", "......."]
    FONT_GLYPHS["J"] = ["....##.", "....##.", "....##.", "....##.", "##..##.", "##..##.", ".####..", "......."]
    FONT_GLYPHS["K"] = ["##...##", "##..##.", "##.##..", "####...", "##.##..", "##..##.", "##...##", "......."]
    FONT_GLYPHS["L"] = ["##.....", "##.....", "##.....", "##.....", "##.....", "##.....", "#######", "......."]
    FONT_GLYPHS["M"] = ["##...##", "###.###", "#######", "##.#.##", "##.#.##", "##...##", "##...##", "......."]
    FONT_GLYPHS["N"] = ["##...##", "###..##", "####.##", "##.####", "##..###", "##...##", "##...##", "......."]
    FONT_GLYPHS["O"] = [".#####.", "##...##", "##...##", "##...##", "##...##", "##...##", ".#####.", "......."]
    FONT_GLYPHS["P"] = ["######.", "##...##", "##...##", "######.", "##.....", "##.....", "##.....", "......."]
    FONT_GLYPHS["Q"] = [".#####.", "##...##", "##...##", "##...##", "##.#.##", "##..###", ".#####.", "......."]
    FONT_GLYPHS["R"] = ["######.", "##...##", "##...##", "######.", "##.##..", "##..##.", "##...##", "......."]
    FONT_GLYPHS["S"] = [".#####.", "##...##", "##.....", ".#####.", ".....##", "##...##", ".#####.", "......."]
    FONT_GLYPHS["T"] = ["#######", "...##..", "...##..", "...##..", "...##..", "...##..", "...##..", "......."]
    FONT_GLYPHS["U"] = ["##...##", "##...##", "##...##", "##...##", "##...##", "##...##", ".#####.", "......."]
    FONT_GLYPHS["V"] = ["##...##", "##...##", "##...##", ".##.##.", ".##.##.", "..###..", "...#...", "......."]
    FONT_GLYPHS["W"] = ["##.#.##", "##.#.##", "##.#.##", "##.#.##", "#######", "###.###", ".#...#.", "......."]
    FONT_GLYPHS["X"] = ["##...##", ".##.##.", "..###..", "...#...", "..###..", ".##.##.", "##...##", "......."]
    FONT_GLYPHS["Y"] = ["##...##", ".##.##.", "..###..", "...##..", "...##..", "...##..", "...##..", "......."]
    FONT_GLYPHS["Z"] = ["#######", ".....##", "....##.", "...##..", "..##...", ".##....", "#######", "......."]
    # Lowercase
    FONT_GLYPHS["a"] = [".......", ".......", ".#####.", ".....##", ".######", "##...##", ".######", "......."]
    FONT_GLYPHS["b"] = ["##.....", "##.....", "######.", "##...##", "##...##", "##...##", "######.", "......."]
    FONT_GLYPHS["c"] = [".......", ".......", ".#####.", "##...##", "##.....", "##...##", ".#####.", "......."]
    FONT_GLYPHS["d"] = [".....##", ".....##", ".######", "##...##", "##...##", "##...##", ".######", "......."]
    FONT_GLYPHS["e"] = [".......", ".......", ".#####.", "##...##", "#######", "##.....", ".#####.", "......."]
    FONT_GLYPHS["f"] = ["..####.", "..##...", "######.", "..##...", "..##...", "..##...", "..##...", "......."]
    FONT_GLYPHS["g"] = [".......", ".......", ".#####.", "##...##", "##...##", ".######", ".....##", ".#####."]
    FONT_GLYPHS["h"] = ["##.....", "##.....", "######.", "##...##", "##...##", "##...##", "##...##", "......."]
    FONT_GLYPHS["i"] = ["..##..", "......", ".###..", "..##..", "..##..", "..##..", "######", "......"]
    FONT_GLYPHS["j"] = ["....##.", ".......", "...###.", "....##.", "....##.", "##..##.", ".####..", "......."]
    FONT_GLYPHS["k"] = ["##.....", "##.....", "##..##.", "##.##..", "####...", "##.##..", "##..##.", "......."]
    FONT_GLYPHS["l"] = [".###..", "..##..", "..##..", "..##..", "..##..", "..##..", "######", "......"]
    FONT_GLYPHS["m"] = [".......", ".......", "######.", "##.#.##", "##.#.##", "##...##", "##...##", "......."]
    FONT_GLYPHS["n"] = [".......", ".......", "######.", "##...##", "##...##", "##...##", "##...##", "......."]
    FONT_GLYPHS["o"] = [".......", ".......", ".#####.", "##...##", "##...##", "##...##", ".#####.", "......."]
    FONT_GLYPHS["p"] = [".......", ".......", "######.", "##...##", "##...##", "######.", "##.....", "##....."]
    FONT_GLYPHS["q"] = [".......", ".......", ".######", "##...##", "##...##", ".######", ".....##", ".....##"]
    FONT_GLYPHS["r"] = ["......", "......", "##.###", "###...", "##....", "##....", "##....", "......"]
    FONT_GLYPHS["s"] = [".......", ".......", ".#####.", "##.....", ".#####.", ".....##", "######.", "......."]
    FONT_GLYPHS["t"] = ["..##..", "..##..", "######", "..##..", "..##..", "..##..", "..##..", "......"]
    FONT_GLYPHS["u"] = [".......", ".......", "##...##", "##...##", "##...##", "##...##", ".######", "......."]
    FONT_GLYPHS["v"] = [".......", ".......", "##...##", "##...##", "##...##", ".##.##.", "..###..", "......."]
    FONT_GLYPHS["w"] = [".......", ".......", "##...##", "##...##", "#######", "###.###", ".#...#.", "......."]
    FONT_GLYPHS["x"] = [".......", ".......", "##...##", ".##.##.", "..###..", ".##.##.", "##...##", "......."]
    FONT_GLYPHS["y"] = [".......", ".......", "##...##", "##...##", ".######", ".....##", ".#####.", "##...##"]
    FONT_GLYPHS["z"] = [".......", ".......", "#######", "....##.", "...##..", "..##...", "#######", "......."]
    # Digits
    FONT_GLYPHS["0"] = [".#####.", "##...##", "##...##", "##...##", "##...##", "##...##", ".#####.", "......."]
    FONT_GLYPHS["1"] = ["..###..", ".####..", "...##..", "...##..", "...##..", "...##..", ".######", "......."]
    FONT_GLYPHS["2"] = [".#####.", "##...##", ".....##", ".#####.", "##.....", "##.....", "#######", "......."]
    FONT_GLYPHS["3"] = [".#####.", "##...##", ".....##", "..####.", ".....##", "##...##", ".#####.", "......."]
    FONT_GLYPHS["4"] = ["##...##", "##...##", "##...##", "#######", ".....##", ".....##", ".....##", "......."]
    FONT_GLYPHS["5"] = ["#######", "##.....", "######.", ".....##", ".....##", "##...##", ".#####.", "......."]
    FONT_GLYPHS["6"] = [".#####.", "##...##", "##.....", "######.", "##...##", "##...##", ".#####.", "......."]
    FONT_GLYPHS["7"] = ["#######", ".....##", "....##.", "...##..", "..##...", "..##...", "..##...", "......."]
    FONT_GLYPHS["8"] = [".#####.", "##...##", "##...##", ".#####.", "##...##", "##...##", ".#####.", "......."]
    FONT_GLYPHS["9"] = [".#####.", "##...##", "##...##", ".######", ".....##", "##...##", ".#####.", "......."]
    # Symbols
    FONT_GLYPHS[":"] = ["...", "...", "##.", "##.", "...", "##.", "##.", "..."]
    FONT_GLYPHS[" "] = ["...", "...", "...", "...", "...", "...", "...", "..."]
    FONT_GLYPHS["-"] = ["....", "....", "....", "####", "....", "....", "....", "...."]
    FONT_GLYPHS["!"] = [".##.", ".##.", ".##.", ".##.", "....", ".##.", ".##.", "...."]

    sorted_chars = sorted(FONT_GLYPHS.keys())
    char_to_idx = {c: i for i, c in enumerate(sorted_chars)}

    glyph_lines = []
    glyph_lines.append("const TitleFontGlyph title_font_glyphs[] PROGMEM = {")
    for ch in sorted_chars:
        rows = FONT_GLYPHS[ch]
        gw = len(rows[0])
        col_bytes = []
        for c in range(gw):
            b = 0
            for r in range(8):
                if rows[r][c] == "#":
                    b |= (1 << r)
            col_bytes.append(b)
        while len(col_bytes) < 7:
            col_bytes.append(0)
        hex_strs = [f"0x{b:02x}" for b in col_bytes]
        safe_char = "\\\x27" if ch == "\x27" else ("\\\\" if ch == "\\" else ch)
        glyph_lines.append(f"  {{ {gw}, {{ {', '.join(hex_strs)} }} }}, // \x27{safe_char}\x27")
    glyph_lines.append("};")

    map_lines = []
    map_lines.append("const uint8_t title_font_map[] PROGMEM = {")
    map_indices = []
    for code in range(32, 123):
        ch = chr(code)
        idx = char_to_idx.get(ch, 255)
        map_indices.append(f"{idx:3d}")
    for i in range(0, len(map_indices), 16):
        map_lines.append("  " + ", ".join(map_indices[i:i+16]) + ",")
    map_lines.append("};")

    font_definitions = "\n".join(glyph_lines) + "\n\n" + "\n".join(map_lines)

    title_constants = ""
    title_array = ""
    if has_title:
        title_constants = """static const uint8_t TITLE_SCREEN_WIDTH     = 128;
static const uint8_t TITLE_SCREEN_HEIGHT    = 64;
static const uint8_t TITLE_WHITEHOLE_X      = 92;
static const uint8_t TITLE_WHITEHOLE_Y      = 30;
static const uint8_t TITLE_FONT_ASCII_MIN   = 32;
static const uint8_t TITLE_FONT_ASCII_MAX   = 122;

struct TitleFontGlyph {
    uint8_t width;
    uint8_t cols[7];
};
"""
        title_array = f"""// Title Screen — 128x64 cover image (whitehole region blanked for animation)
{format_c_array("title_screen_sprite", t_bytes)}

// =============================================================================
// TITLE FONT — Bold 7-pixel pixel font matching cover title typography
// =============================================================================
{font_definitions}
"""

    header_content = f"""// =============================================================================
// sprites.h — Supermassive Whitehole PROGMEM Sprites
// =============================================================================
// Generated automatically by scripts/convert_sprites.py.
// Do NOT edit raw hex arrays by hand — edit the PNGs in assets/ and re-run:
//   python3 scripts/convert_sprites.py
//
// FORMAT:
//   Arduboy column-major vertical byte order (SSD1306 OLED page layout).
//   First 2 bytes: width, height.
//   Followed by (height / 8) * width bytes per frame.
//   Compatible with Sprites::drawSelfMasked() and Sprites::drawOverwrite().
// =============================================================================

#ifndef SPRITES_H
#define SPRITES_H

#include <stdint.h>
#include <avr/pgmspace.h>

// =============================================================================
// SPRITE DIMENSIONS & CONSTANTS
// =============================================================================

static const uint8_t PLAYER_SPRITE_WIDTH   = 16;
static const uint8_t PLAYER_SPRITE_HEIGHT  = 16;

static const uint8_t WHITEHOLE_SPRITE_WIDTH  = 32;
static const uint8_t WHITEHOLE_SPRITE_HEIGHT = 32;
static const uint8_t WHITEHOLE_FRAME_COUNT   = 4;

static const uint8_t ITEM_SPRITE_WIDTH     = 16;
static const uint8_t ITEM_SPRITE_HEIGHT    = 16;

{title_constants}
// =============================================================================
// ITEM SPRITESHEET FRAME INDICES (items_16.png — 9 cols x 2 rows = 18 items)
// =============================================================================
// Row 0:
//   0: Coffee Mug     1: Croissant     2: Burger        3: Pizza
//   4: Donut          5: Bottle        6: Apple         7: Sushi
//   8: Ice Cream
// Row 1:
//   9: Fries         10: Taco         11: Soda Can     12: Gem / Collectible
//  13: Cake          14: Sandwich     15: Cherry       16: Candy
//  17: Star

static const uint8_t SPRITE_ITEM_COFFEE      = 0;
static const uint8_t SPRITE_ITEM_CROISSANT   = 1;
static const uint8_t SPRITE_ITEM_BURGER      = 2;
static const uint8_t SPRITE_ITEM_PIZZA       = 3;
static const uint8_t SPRITE_ITEM_DONUT       = 4;
static const uint8_t SPRITE_ITEM_APPLE       = 6;
static const uint8_t SPRITE_ITEM_ICECREAM    = 8;
static const uint8_t SPRITE_ITEM_FRIES       = 9;
static const uint8_t SPRITE_ITEM_TACO        = 10;
static const uint8_t SPRITE_ITEM_COLLECTIBLE = 12;
static const uint8_t SPRITE_ITEM_CANDY       = 16;
static const uint8_t SPRITE_ITEM_STAR        = 17;

// =============================================================================
// PROGMEM SPRITE DATA ARRAYS
// =============================================================================

// Player — 16x16 single frame
{format_c_array("player_sprite", p_bytes)}

// Whitehole — 32x32 animated (4 swirling frames)
{format_c_array("whitehole_sprite", wh_bytes)}

// Items — 16x16 spritesheet (18 items)
{format_c_array("items_sprites", it_bytes)}

{title_array}
#endif // SPRITES_H
"""

    with open(OUTPUT_HEADER, "w") as f:
        f.write(header_content)

    total_bytes = len(p_bytes) + len(wh_bytes) + len(it_bytes) + len(t_bytes)
    print(f"Successfully generated {OUTPUT_HEADER} ({total_bytes} bytes in Flash)")


if __name__ == "__main__":
    main()
