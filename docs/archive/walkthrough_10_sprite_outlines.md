# Walkthrough — Sprite Background, Outline & Transparency Configuration

We have eliminated transparency bleed through item and player sprites by introducing solid black backgrounds and configurable outlines (sticker-like visuals), while keeping the swirling whitehole naturally transparent.

## Visual Comparison

````carousel
![Previous Transparent Sprites — Grid bleeding through](/home/dorito/.gemini/antigravity-ide/brain/68273b2e-9695-4636-af21-100681b62600/preview_current.png)
<!-- slide -->
![Default: Black Background + 1px Black Outline](/home/dorito/.gemini/antigravity-ide/brain/68273b2e-9695-4636-af21-100681b62600/preview_black_outline_1px.png)
<!-- slide -->
![Option: Black Background + 2px Black Outline](/home/dorito/.gemini/antigravity-ide/brain/68273b2e-9695-4636-af21-100681b62600/preview_black_outline_2px.png)
<!-- slide -->
![Option: Black Background + 1px White Sticker Outline](/home/dorito/.gemini/antigravity-ide/brain/68273b2e-9695-4636-af21-100681b62600/preview_white_outline_1px.png)
<!-- slide -->
![Option: Solid Black Background with No Outline](/home/dorito/.gemini/antigravity-ide/brain/68273b2e-9695-4636-af21-100681b62600/preview_no_outline_black_bg.png)
<!-- slide -->
![All 18 Items & Player with Sticker Outlines](/home/dorito/.gemini/antigravity-ide/brain/68273b2e-9695-4636-af21-100681b62600/sheet_cross.png)
````

---

## Changes Made

### 1. Sprite Conversion Pipeline ([convert_sprites.py](file:///home/dorito/Developer/arduboy/supermassive-whitehole/scripts/convert_sprites.py))
- Added `image_to_frames_bool`, `extract_interior_mask` (exterior flood fill), and `dilate_mask` (4-way cardinal dilation).
- Automatically produces:
  - `player_sprite` (bitmap art), `player_mask` (interior silhouette), `player_outline` (1px dilated border)
  - `items_sprites` (bitmap art), `items_masks` (interior silhouette), `items_outlines` (1px dilated border)
  - `whitehole_sprite` (bitmap art), `whitehole_mask` (interior silhouette)
- Output to [assets/sprites.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/assets/sprites.h).

### 2. HAL Renderer API ([renderer.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/hal/renderer.h) & [arduboy_renderer.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/platform/arduboy/arduboy_renderer.h))
- Added `drawExternalMask` (clears buffer using mask, writes bitmap art).
- Added `drawErase` (clears buffer bits where bitmap is 1, leaving 0-bits untouched).

### 3. Central Configuration ([config.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/config.h))
Added type-safe enums and per-actor constants:
```cpp
enum SpriteAlphaMode : uint8_t {
    SPRITE_ALPHA_TRANSPARENT = 0,
    SPRITE_ALPHA_OPAQUE      = 1,
};

enum SpriteOutlineMode : uint8_t {
    SPRITE_OUTLINE_NONE  = 0,
    SPRITE_OUTLINE_BLACK = 1,
    SPRITE_OUTLINE_WHITE = 2,
};

enum SpriteOutlineRadius : uint8_t {
    SPRITE_OUTLINE_1PX = 1,
    SPRITE_OUTLINE_2PX = 2,
};

// Player Settings (Default: Solid black background + 1px black outline)
static const SpriteAlphaMode     PLAYER_ALPHA_MODE     = SPRITE_ALPHA_OPAQUE;
static const SpriteOutlineMode   PLAYER_OUTLINE_MODE   = SPRITE_OUTLINE_BLACK;
static const SpriteOutlineRadius PLAYER_OUTLINE_RADIUS = SPRITE_OUTLINE_1PX;

// Entity / Item Settings (Default: Solid black background + 1px black outline)
static const SpriteAlphaMode     ENTITY_ALPHA_MODE     = SPRITE_ALPHA_OPAQUE;
static const SpriteOutlineMode   ENTITY_OUTLINE_MODE   = SPRITE_OUTLINE_BLACK;
static const SpriteOutlineRadius ENTITY_OUTLINE_RADIUS = SPRITE_OUTLINE_1PX;

// Whitehole Settings (Default: Transparent + no outline)
static const SpriteAlphaMode     WHITEHOLE_ALPHA_MODE   = SPRITE_ALPHA_TRANSPARENT;
static const SpriteOutlineMode   WHITEHOLE_OUTLINE_MODE = SPRITE_OUTLINE_NONE;
```

### 4. Game Rendering Pipeline ([game.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.h) & [game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp))
- Implemented `Game::drawSpriteWithConfig(...)`:
  - **`SPRITE_ALPHA_TRANSPARENT`**: Direct `renderer.drawSelfMasked(x, y, bitmap, frame)`.
  - **`SPRITE_ALPHA_OPAQUE` + `SPRITE_OUTLINE_BLACK`**: `drawErase(outline)` then `drawSelfMasked(bitmap)`.
  - **`SPRITE_ALPHA_OPAQUE` + `SPRITE_OUTLINE_WHITE`**: `drawSelfMasked(outline)` then `drawExternalMask(bitmap, mask)`.
  - **`SPRITE_ALPHA_OPAQUE` + `SPRITE_OUTLINE_NONE`**: `drawExternalMask(bitmap, mask)`.
- Applied to:
  - `renderEntities(...)`
  - `renderPlayer(...)` (respecting player blinking debuff)
  - `renderBlackhole(...)` (retaining full whitehole transparency)

### 5. Task Tracking ([docs/TODO.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/TODO.md))
- Checked off task:
  ```markdown
  - [x] Fix sprites transparency to no transparency and add outline.
  ```

---

## Verification Results

### Build & Memory Budget
```text
Sketch uses 19114 bytes (66%) of program storage space. Maximum is 28672 bytes.
Global variables use 1515 bytes (59%) of dynamic memory, leaving 1045 bytes for local variables. Maximum is 2560 bytes.
```
- **Flash storage**: 9,558 bytes free (plenty of headroom).
- **RAM usage**: 0 additional bytes consumed (all sprite masks and outlines reside in `PROGMEM`).
- **Modes verified**: Compilation and artifact sync tested cleanly across `SPRITE_OUTLINE_BLACK`, `SPRITE_OUTLINE_WHITE`, and `SPRITE_OUTLINE_NONE`.
