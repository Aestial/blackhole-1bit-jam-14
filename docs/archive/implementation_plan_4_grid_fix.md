# Fix Pseudo-3D Ground Plane — Objects Flying Above Horizon

## Problem

Entities (whitehole, food items, collectibles) rendered via `worldToScreenY()` can project **above the horizon line** ($y < 14$) when their world-Y position places them far "into the distance" ($z \le 0$). This breaks the ground-plane illusion — sprites appear to take flight into the celestial sky instead of receding along the ground and disappearing at the horizon.

The core issue is that the current projection formula allows any $z \le 0$ to produce screen-Y values above `HORIZON_Y`, where they visually detach from the ground grid entirely.

---

## Design Decision Required

You outlined two approaches. Here's a technical analysis of both:

### Option A: Real-Time Sprite Scaling with Distance (Pseudo-3D Racing Style)

Scale each sprite based on its depth $z$, shrinking sprites as they approach the horizon and clipping/fading them away.

**Feasibility on ATmega32u4:**

> [!WARNING]
> Software bitmap scaling is **expensive** on AVR. The Arduboy's `Sprites::drawSelfMasked()` draws pre-formatted PROGMEM bitmaps at 1:1 scale using direct page-aligned SSD1306 byte blitting. There is no hardware-accelerated scaling.
>
> A software nearest-neighbor scaler for a 16×16 sprite at 50% scale (8×8 output) requires:
> - Per-pixel source coordinate lookup: `srcX = (dstX * srcW) / dstW` — one 16-bit multiply + divide per pixel
> - 64 pixels × ~10 cycles = **~640 cycles per sprite** at 50% scale
> - 12 items + 1 player + 1 whitehole = **14 sprites** worst case
> - Total: ~14 × 640 = **~9,000 cycles per frame** for scaling alone
>
> At 60 FPS with a 16 MHz clock, we have **266,667 cycles/frame** total budget. 9,000 cycles is **~3.4%** — this is actually **feasible** for the CPU, but there are complications:
>
> 1. **Code size**: A generic sprite scaler adds ~300-500 bytes of Flash (we're at 53%).
> 2. **The whitehole is 32×32 with 4 animation frames** — scaling a 32×32 sprite at arbitrary sizes is 4× more work per sprite.
> 3. **Masking**: `drawSelfMasked` transparency handling must be replicated in the scaler.
> 4. **Visual quality**: At 1-bit monochrome with nearest-neighbor, scaling below ~50% produces ugly pixel artifacts. A 16×16 sprite scaled to 4×4 loses almost all recognizability.

**Verdict**: Technically possible but adds significant complexity, and the visual payoff at 1-bit resolution is questionable. Sprites become unrecognizable below ~8×8 pixels. This is more appropriate as a **polish item for M5** once core gameplay is locked.

---

### Option B: Flatten Camera Angle — Eliminate Visible Horizon (Recommended)

Lower the virtual camera so the horizon line is **above the top of the screen** (or at $y = 0$). The entire 128×64 screen becomes the ground plane — no sky, no horizon. Objects that move "into the distance" simply get closer to the top of the screen and eventually scroll off-screen naturally.

This is exactly how classic pseudo-3D games like *OutRun*, *Rad Racer*, and *Road Rash* handle it when the road fills the entire screen — the horizon is at or above the top edge.

**Advantages:**
- ✅ **Zero additional CPU cost** — same projection math, just different constants
- ✅ **No sprite scaling needed** — objects naturally exit at the screen top edge
- ✅ **No visual artifacts** — sprites never appear "in the sky"
- ✅ **More ground area** — the full 64px height is usable play area
- ✅ **Grid fills entire screen** — stronger visual presence

**Trade-off:**
- ⚠️ The open sky area above the horizon (currently used for clean HUD/score display) is lost. The HUD will need to overlay on top of the grid, which requires a small black backing rectangle for readability.

---

## Recommendation

> [!IMPORTANT]
> **I recommend Option B (flatten camera) as the immediate fix**, with Option A (sprite scaling) as an optional polish enhancement for M5.

Option B solves the "flying objects" problem completely with zero performance cost, and the HUD overlay is a minor adjustment. Option A can be added later as a visual depth cue once sprites and gameplay are finalized.

---

## Open Questions

1. **Horizon position**: Should the horizon be moved to exactly $y = 0$ (completely invisible, full-screen ground) or to $y = -10$ or lower (leaving a thin sliver that clips out)? Moving it lower creates a more dramatic perspective convergence at the top of the screen.

2. **HUD placement**: With no sky area, the score overlay currently at top-right will sit on top of grid lines. Should we:
   - Add a small filled black rectangle behind the score text?
   - Move the HUD to the bottom of the screen instead?
   - Keep it top-right with a black backing?

3. **Sprite scaling for M5**: Would you like me to stub out a `worldToScale(worldY)` function now that returns a scale factor (256 = 1.0, 128 = 0.5, etc.) for future use, even if we don't use it for rendering yet?

---

## Proposed Changes

### Perspective Constants

#### [MODIFY] [config.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/config.h)
- Change `HORIZON_Y` from `14` to `0` (or a small negative value like `-8`)
- Adjust `GROUND_DEPTH_PLAYER` so the player's screen-Y position maps correctly to the vertical center (~32)
- Adjust `BASE_SPACING_X` / `TOP_SPACING_X` to tune the convergence now that the vanishing point is at/above the screen top

---

### Projection Functions

#### [MODIFY] [world.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/world.cpp)
- Update `worldToScreenY()`: Adjust the quadratic curve so that $z = 0$ maps to $y = 0$ (or the new `HORIZON_Y`) and $z = \text{GROUND\_DEPTH\_PLAYER}$ maps to $y \approx 32$
- Update `worldToScreenX()`: Adjust the horizontal convergence factor to match the new vanishing point
- Entities with $sy < 0$ are simply off-screen and get culled naturally by existing bounds checks

---

### Background Grid Rendering

#### [MODIFY] [game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp)
- `renderBackground()`: Adjust horizon line draw position (at $y = 0$ it's effectively invisible, or draw it only if `HORIZON_Y > 0`)
- Perspective rays now converge toward the top edge of the screen instead of $y = 14$
- Depth lines fill the full 64px height
- `renderHUD()`: Add a small black backing rectangle behind the score text for readability over grid lines

---

### Entity Rendering & Culling

#### [MODIFY] [game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp)
- `renderEntities()`, `renderPlayer()`, `renderBlackhole()`: Update horizon culling from `HORIZON_Y - spriteHeight` to simply `< -spriteHeight` (standard off-screen check)
- Sprites that approach the horizon naturally exit at the top of the screen

---

### Documentation

#### [MODIFY] [gdd.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/gdd.md)
- Update the "Infinite Plane" section to describe the full-screen ground grid with no visible horizon

#### [MODIFY] [architecture.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/architecture.md)
- Update M1 description to note the flattened camera angle

---

## Verification Plan

### Build
```bash
./build.sh --build-only
```
- Must compile with 0 errors, 0 warnings
- Flash and SRAM usage should remain very close to current (15,348 / 1,467)

### Visual Verification
- Web emulator at `http://localhost:8080`
- Confirm: grid fills entire screen, no visible horizon line, perspective rays converge at screen top
- Confirm: whitehole enemy recedes toward the top of the screen when player moves south, never floats above grid
- Confirm: HUD score is readable with black backing rectangle
- Confirm: player position feels centered and gameplay feel is preserved
