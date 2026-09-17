# Implementation Plan — Sprite Outlines, Backgrounds, and Transparency Configuration

Provide configurable transparency (1-bit alpha: Opaque vs Transparent) and outline settings (Black Outline, White Outline, No Outline) for player, entity sprites, and whitehole. The default is set to **Solid Black Background + 1px Black Outline** (`SPRITE_OUTLINE_1PX`) to ensure perspective grid lines never bleed through actors while keeping flash memory usage minimal.

## User Review Required

> [!IMPORTANT]
> **Configuration Setup in [config.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/config.h)**:
> ```cpp
> // 1. Transparency / 1-bit Alpha Mode
> enum SpriteAlphaMode : uint8_t {
>     SPRITE_ALPHA_TRANSPARENT = 0, // Transparent background (grid shows through)
>     SPRITE_ALPHA_OPAQUE      = 1, // Solid black background (blocks grid lines)
> };
> 
> // 2. Outline Mode (with solid black background)
> enum SpriteOutlineMode : uint8_t {
>     SPRITE_OUTLINE_NONE  = 0, // No outline border (mask cutout only)
>     SPRITE_OUTLINE_BLACK = 1, // Black outline border halo (cuts grid lines)
>     SPRITE_OUTLINE_WHITE = 2, // White outline border (die-cut sticker border)
> };
> 
> // 3. Outline Radius / Thickness
> enum SpriteOutlineRadius : uint8_t {
>     SPRITE_OUTLINE_1PX = 1, // Default: memory-efficient 1px border
>     SPRITE_OUTLINE_2PX = 2, // Optional 2px border for exception entities
> };
> 
> // --- Player Settings ---
> static const SpriteAlphaMode     PLAYER_ALPHA_MODE     = SPRITE_ALPHA_OPAQUE;
> static const SpriteOutlineMode   PLAYER_OUTLINE_MODE   = SPRITE_OUTLINE_BLACK;
> static const SpriteOutlineRadius PLAYER_OUTLINE_RADIUS = SPRITE_OUTLINE_1PX;
> 
> // --- Entities / Items Settings ---
> static const SpriteAlphaMode     ENTITY_ALPHA_MODE     = SPRITE_ALPHA_OPAQUE;
> static const SpriteOutlineMode   ENTITY_OUTLINE_MODE   = SPRITE_OUTLINE_BLACK;
> static const SpriteOutlineRadius ENTITY_OUTLINE_RADIUS = SPRITE_OUTLINE_1PX;
> 
> // --- Whitehole Settings (Unique case: default transparent, no outline) ---
> static const SpriteAlphaMode     WHITEHOLE_ALPHA_MODE   = SPRITE_ALPHA_TRANSPARENT;
> static const SpriteOutlineMode   WHITEHOLE_OUTLINE_MODE = SPRITE_OUTLINE_NONE;
> ```

## Proposed Changes

### Sprite Conversion Tooling & Assets

#### [MODIFY] [convert_sprites.py](file:///home/dorito/Developer/arduboy/supermassive-whitehole/scripts/convert_sprites.py)
- Update script to generate:
  - Artwork bitmaps (`player_sprite`, `items_sprites`, `whitehole_sprite`)
  - Interior silhouette masks (`player_mask`, `items_masks`, `whitehole_mask`) via flood-fill exterior detection
  - 1px dilated outline masks (`player_outline`, `items_outlines`) by default to save flash memory
- Generate PROGMEM C arrays to [assets/sprites.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/assets/sprites.h).

#### [MODIFY] [sprites.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/assets/sprites.h)
- Regenerated with `player_mask`, `player_outline`, `items_masks`, `items_outlines`, and `whitehole_mask`.

---

### Hardware Abstraction Layer (HAL)

#### [MODIFY] [renderer.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/hal/renderer.h)
- Add HAL API contract declarations:
  - `void drawExternalMask(int16_t x, int16_t y, const uint8_t* bitmap, const uint8_t* mask, uint8_t frame, uint8_t mask_frame = 0);`
  - `void drawErase(int16_t x, int16_t y, const uint8_t* bitmap, uint8_t frame);`

#### [MODIFY] [arduboy_renderer.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/platform/arduboy/arduboy_renderer.h)
- Implement `drawExternalMask` wrapping `Sprites::drawExternalMask`.
- Implement `drawErase` wrapping `Sprites::drawErase`.

---

### Game Logic & Rendering

#### [MODIFY] [config.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/config.h)
- Add enums: `SpriteAlphaMode`, `SpriteOutlineMode`, `SpriteOutlineRadius`.
- Add actor-specific constants (defaulting to 1px black outline + solid black background for player and entities; transparent for whitehole).

#### [MODIFY] [game.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.h)
- Declare generic helper:
  ```cpp
  void drawSpriteWithConfig(HalRenderer& renderer, int16_t x, int16_t y,
                            const uint8_t* bitmap, const uint8_t* mask,
                            const uint8_t* outline, uint8_t frame,
                            SpriteAlphaMode alphaMode,
                            SpriteOutlineMode outlineMode);
  ```

#### [MODIFY] [game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp)
- Implement `drawSpriteWithConfig`:
  - If `alphaMode == SPRITE_ALPHA_TRANSPARENT`:
    `renderer.drawSelfMasked(x, y, bitmap, frame)`.
  - If `alphaMode == SPRITE_ALPHA_OPAQUE`:
    - `SPRITE_OUTLINE_BLACK`:
      `renderer.drawErase(x, y, outline, frame)` followed by `renderer.drawSelfMasked(x, y, bitmap, frame)`.
    - `SPRITE_OUTLINE_WHITE`:
      `renderer.drawSelfMasked(x, y, outline, frame)` followed by `renderer.drawExternalMask(x, y, bitmap, mask, frame, frame)`.
    - `SPRITE_OUTLINE_NONE`:
      `renderer.drawExternalMask(x, y, bitmap, mask, frame, frame)`.
- Use `drawSpriteWithConfig` in:
  - `renderEntities`: draws each item with `ENTITY_ALPHA_MODE` and `ENTITY_OUTLINE_MODE`.
  - `renderPlayer`: draws player with `PLAYER_ALPHA_MODE` and `PLAYER_OUTLINE_MODE`.
  - `renderBlackhole`: draws whitehole with `WHITEHOLE_ALPHA_MODE` and `WHITEHOLE_OUTLINE_MODE`.

---

### Documentation & Project Tracking

#### [MODIFY] [docs/TODO.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/TODO.md)
- Check off task: `- [x] Fix sprites transparency to no transparency and add outline.`

## Verification Plan

### Automated Build Verification
1. Run `python3 scripts/convert_sprites.py` to generate `assets/sprites.h`.
2. Run `./build.sh --build-only` to ensure AVR compilation passes and check program storage and RAM usage.
3. Verify compilation when toggling between `SPRITE_OUTLINE_BLACK`, `SPRITE_OUTLINE_WHITE`, and `SPRITE_OUTLINE_NONE`.

### Visual & Emulation Verification
1. Run `./build.sh --build-only` and launch or verify generated artifacts.
2. Confirm items and player have solid black interior and 1px black outline cutting grid lines.
3. Confirm whitehole remains transparent.
