# Walkthrough — Supermassive Whitehole & Custom Sprite Pipeline

We have renamed the project to **Supermassive Whitehole**, built an automated sprite conversion pipeline, integrated custom PROGMEM sprites for Player, Whitehole (4 animated frames), and Food/Collectible entities (including the new Ice Cream item), and prepared the project for renaming to `supermassive-whitehole`.

---

## 1. Summary of Changes

### Project Branding & Renaming
- **Title Screen**: Updated in [src/game/game.cpp](file:///home/dorito/Developer/arduboy/blackhole/src/game/game.cpp) to render `"SUPERMASSIVE"` and `"WHITEHOLE"`.
- **Web Player**: Updated `<title>Supermassive Whitehole - Arduboy</title>` in [html5/index.html](file:///home/dorito/Developer/arduboy/blackhole/html5/index.html) and [dist/web/index.html](file:///home/dorito/Developer/arduboy/blackhole/dist/web/index.html).
- **Distribution Packages**: [scripts/build_and_run.sh](file:///home/dorito/Developer/arduboy/blackhole/scripts/build_and_run.sh) and [scripts/package_dist.sh](file:///home/dorito/Developer/arduboy/blackhole/scripts/package_dist.sh) build `dist/whitehole.hex` and `dist/whitehole-web.zip` (and maintain legacy mirrors for backwards compatibility).
- **Documentation**: Updated [README.md](file:///home/dorito/Developer/arduboy/blackhole/README.md), [dist/README.md](file:///home/dorito/Developer/arduboy/blackhole/dist/README.md), [docs/gdd.md](file:///home/dorito/Developer/arduboy/blackhole/docs/gdd.md), and [docs/distribution.md](file:///home/dorito/Developer/arduboy/blackhole/docs/distribution.md).

---

### Custom Sprite Pipeline & Integration

#### 1. Automated Converter ([scripts/convert_sprites.py](file:///home/dorito/Developer/arduboy/blackhole/scripts/convert_sprites.py))
- Reads PNGs from `assets/` and encodes them into column-major vertical byte order for Arduboy OLED display pages.
- Auto-generates [assets/sprites.h](file:///home/dorito/Developer/arduboy/blackhole/assets/sprites.h) with dimension constants, frame definitions, and `PROGMEM` data arrays.
- Sprites converted:
  - **Player** (`assets/player_static.png`): 16×16 single frame (34 bytes Flash).
  - **Whitehole** (`assets/whitehole_32.png`): 32×32 with 4 animated swirling frames (514 bytes Flash).
  - **Items** (`assets/items_16.png`): 16×16 spritesheet with 18 items (578 bytes Flash).
  - Total Flash used for all sprites: **1,126 bytes**.

#### 2. Added Entity: Ice Cream (`ENTITY_FOOD_ICECREAM`)
- Added from `items_16.png` (col 8, row 0 / frame 8).
- Mechanics:
  - Hitbox: `FOOD_ICECREAM_SIZE` (6px).
  - Brain Freeze Debuff: Duration `FOOD_ICECREAM_SLOW_DURATION = 120` frames (2.0 seconds), intensity `FOOD_ICECREAM_SLOW_INTENSITY = 60` (-60% speed).
- Updated in:
  - [src/game/config.h](file:///home/dorito/Developer/arduboy/blackhole/src/game/config.h)
  - [src/game/entity.h](file:///home/dorito/Developer/arduboy/blackhole/src/game/entity.h)
  - [src/game/entity.cpp](file:///home/dorito/Developer/arduboy/blackhole/src/game/entity.cpp)
  - [src/game/player.cpp](file:///home/dorito/Developer/arduboy/blackhole/src/game/player.cpp)

#### 3. HAL & Game Rendering
- **HAL Contract** ([src/hal/renderer.h](file:///home/dorito/Developer/arduboy/blackhole/src/hal/renderer.h)): Added `drawSelfMasked(x, y, bitmap, frame)` and `drawOverwrite(x, y, bitmap, frame)`.
- **Concrete HAL** ([src/platform/arduboy/arduboy_renderer.h](file:///home/dorito/Developer/arduboy/blackhole/src/platform/arduboy/arduboy_renderer.h)): Wraps Arduboy2's `Sprites::drawSelfMasked` and `Sprites::drawOverwrite`.
- **Game Dispatch** ([src/game/game.cpp](file:///home/dorito/Developer/arduboy/blackhole/src/game/game.cpp)):
  - `renderPlayer()`: Draws 16×16 `player_sprite` centered on screen. Added visual blinking feedback while slowed.
  - `renderBlackhole()`: Draws 32×32 `whitehole_sprite`, smoothly cycling through the 4 swirling animation frames using `world.bhSpin`.
  - `renderEntities()`: Draws each entity using its corresponding frame from `items_sprites` (Pizza = 3, Burger = 2, Donut = 4, Ice Cream = 8, Collectible Gem = 12).

---

### Dynamic Sketch Detection for Renaming
- [scripts/build_and_run.sh](file:///home/dorito/Developer/arduboy/blackhole/scripts/build_and_run.sh) now dynamically discovers `*.ino` in the root folder.
- Whether the sketch is named `blackhole.ino` or `supermassive-whitehole.ino`, `./build.sh` will compile and sync binaries automatically!

---

## 2. Verification Results

### Compilation Check
```bash
./build.sh --package --build-only
```
Output:
```text
======================================================
   Supermassive Whitehole — Arduboy Build Pipeline    
======================================================
[INFO] Compiling sketch with FQBN: arduboy-homemade:avr:arduboy...
Sketch uses 12244 bytes (42%) of program storage space. Maximum is 28672 bytes.
Global variables use 1475 bytes (57%) of dynamic memory, leaving 1085 bytes for local variables. Maximum is 2560 bytes.
[SUCCESS] Compilation succeeded!
[INFO] Synchronizing distribution artifacts...
[SUCCESS] Synchronized dist/whitehole.hex (29190 bytes)
[SUCCESS] Synchronized dist/web/ArduboyProject.hex
[INFO] Creating distribution package for itch.io...
Packaging ProjectABE Web Distribution for itch.io...
[SUCCESS] Created dist/whitehole-web.zip (476 KB)
[SUCCESS] Mirrored to dist/blackhole-web.zip
```
- Flash memory usage: **12,244 bytes (42%)** — over 16 KB still free!
- RAM usage: **1,475 bytes (57%)** — 1,085 bytes free (identical to before sprites were added, zero RAM overhead).
- Zero compiler errors or warnings.

---

## 3. Guide for Renaming Directory to `supermassive-whitehole`

When you are ready to update the directory name on disk:

```bash
# 1. Rename the directory
mv /home/dorito/Developer/arduboy/blackhole /home/dorito/Developer/arduboy/supermassive-whitehole

# 2. Enter new directory
cd /home/dorito/Developer/arduboy/supermassive-whitehole

# 3. Rename the primary .ino file to match the directory name (required by Arduino)
mv blackhole.ino supermassive-whitehole.ino

# 4. Build and test
./build.sh
```

Because all plans, architecture guides, and walkthroughs are stored right inside `docs/`:
- `docs/implementation_plan.md`
- `docs/walkthrough.md`
- `docs/architecture.md`
- `docs/gdd.md`
- `docs/distribution.md`

Your complete project history and specifications remain intact in git even if the IDE re-indexes the new workspace!
