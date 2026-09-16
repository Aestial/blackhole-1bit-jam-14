# Supermassive Whitehole — 1-Bit Jam 14

An action-arcade evasion game for Arduboy and HTML5 web browsers. Dodge irresistible snacks and flee a pursuing supermassive whitehole on a top-down infinite plane!

## Features
- **1-Bit Jam 14**: Built exclusively for Arduboy monochrome OLED (128x64).
- **Custom Sprites**: Custom 16x16 player character, 4-frame animated 32x32 swirling whitehole, and 16x16 item sprites (Pizza, Burger, Donut, Ice Cream brain-freeze, Collectibles).
- **Automated Pipeline**: 1-click build, local ProjectABE web emulation, and automated itch.io release packaging.

## Quick Start
```bash
# Build sketch and test in ProjectABE web emulator
./build.sh

# Run emulator with Arduboy casing skin
./build.sh --skin arduboy

# Build binaries only without launching browser
./build.sh --build-only

# Package release zip for itch.io
./build.sh --package
```

## Adding Custom Sprites
1. Place or edit your PNGs in `assets/`:
   - `player_static.png` (16x16)
   - `whitehole_32.png` (128x32 horizontal spritesheet, 4 frames of 32x32)
   - `items_16.png` (144x32 spritesheet, 18 items of 16x16)
2. Run the sprite converter:
   ```bash
   python3 scripts/convert_sprites.py
   ```
3. Compile with `./build.sh --build-only`.

