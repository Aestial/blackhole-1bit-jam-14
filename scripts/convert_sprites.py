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

#endif // SPRITES_H
"""

    with open(OUTPUT_HEADER, "w") as f:
        f.write(header_content)

    total_bytes = len(p_bytes) + len(wh_bytes) + len(it_bytes)
    print(f"Successfully generated {OUTPUT_HEADER} ({total_bytes} bytes in Flash)")


if __name__ == "__main__":
    main()
