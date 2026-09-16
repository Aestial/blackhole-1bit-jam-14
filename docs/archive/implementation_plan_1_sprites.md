# Implementation Plan — Rename to Supermassive Whitehole & Custom Sprite Pipeline

Rename the project entirely to **Supermassive Whitehole**, establish the custom Arduboy sprite workflow, automate sprite conversion via Python, and integrate the custom assets (`player_static.png`, `whitehole_32.png`, `items_16.png`) into the rendering pipeline.

---

## 1. Project-Wide Renaming to "Supermassive Whitehole"

- **Branding & UI**:
  - Title screen: `Game::renderTitle` prints `"SUPERMASSIVE"` and `"WHITEHOLE"`.
  - HTML5 Web emulator: `<title>Supermassive Whitehole - Arduboy</title>` in `html5/index.html` and `dist/web/index.html`.
  - Root `README.md` and `dist/README.md`: Updated to Supermassive Whitehole with full instructions.
- **Build & Distribution**:
  - `scripts/build_and_run.sh`: Dynamically discovers any `*.ino` file (e.g. `supermassive-whitehole.ino` or `blackhole.ino`).
  - Produces both `dist/whitehole.hex` and `dist/${SKETCH_NAME}.hex`.
  - `scripts/package_dist.sh`: Builds `dist/whitehole-web.zip` (and mirrors `dist/blackhole-web.zip`).
- **Documentation**:
  - `docs/gdd.md`, `docs/architecture.md`, `docs/distribution.md` updated.

---

## 2. Custom Sprites Architecture & Workflow

### Arduboy Hardware Principles:
1. **Vertical Paging (SSD1306 OLED)**:
   - Screen resolution: 128×64 monochrome (1-bit).
   - Video RAM is split into 8 vertical pages (each 8 pixels tall).
   - Sprites must be stored in column-major vertical byte format: bit 0 at top, bit 7 at bottom.
2. **Flash Memory (`PROGMEM`)**:
   - The ATmega32U4 has only 2.5 KB RAM and 28.6 KB Flash.
   - All sprite data must use the `PROGMEM` attribute.
3. **Arduboy2 `Sprites` Class**:
   - `Sprites::drawSelfMasked(x, y, bitmap, frame)`: Draws white pixels (1s) to the screen, leaving black pixels (0s) transparent. Extremely lightweight and fast.
   - `Sprites::drawOverwrite(x, y, bitmap, frame)`: Completely overwrites buffer bytes with sprite frame.
   - Array layout: First two bytes are `width` and `height`, followed by `width * (height / 8)` bytes per frame.

---

## 3. Implemented Components

### Sprite Converter (`scripts/convert_sprites.py`)
- Python script reading:
  - `assets/player_static.png` (16×16) → `player_sprite` (1 frame, 34 bytes)
  - `assets/whitehole_32.png` (128×32) → `whitehole_sprite` (4 frames of 32×32, 514 bytes)
  - `assets/items_16.png` (144×32) → `items_sprites` (18 frames of 16×16, 578 bytes)
- Auto-generates `assets/sprites.h` with dimensions, frame constants, and PROGMEM arrays.

### Entities & Food Mechanics
- Added `ENTITY_FOOD_ICECREAM` (Ice Cream Cone from items_16 col 8, row 0).
- Ice Cream causes brain freeze: duration 120 frames (2.0s), speed reduction -60%.
- Other items mapped:
  - Pizza: col 3, row 0 (light snack, -15%)
  - Burger: col 2, row 0 (medium meal, -30%)
  - Donut: col 4, row 0 (heavy slow, -50%)
  - Collectible: col 3, row 1 (Gem/Crystal, +score)

### HAL & Rendering
- Updated `HalRenderer` contract in `src/hal/renderer.h`.
- Implemented `drawSelfMasked` and `drawOverwrite` in `ArduboyRenderer` (`src/platform/arduboy/arduboy_renderer.h`).
- Updated `Game::renderPlayer`: draws 16×16 player sprite, blinks when slowed.
- Updated `Game::renderBlackhole`: draws animated 32×32 swirling whitehole using `world.bhSpin`.
- Updated `Game::renderEntities`: draws each active entity using its custom item sprite frame.

---

## 4. Renaming Directory & Sketch (.ino)

When renaming the directory from `blackhole` to `supermassive-whitehole`:
```bash
# 1. Rename directory
mv /path/to/blackhole /path/to/supermassive-whitehole

# 2. Rename primary sketch file to match directory name
mv supermassive-whitehole/blackhole.ino supermassive-whitehole/supermassive-whitehole.ino

# 3. Build sketch immediately
cd supermassive-whitehole
./build.sh
```
`build_and_run.sh` dynamically detects `*.ino`, so it will seamlessly build `supermassive-whitehole.ino.hex` without requiring any code or script modifications.
