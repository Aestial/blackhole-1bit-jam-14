# M3–M4 Implementation Plan: Gravity Effects, Particles & Distorted Grid

## Overview

This plan covers two milestones for *Supermassive Whitehole*:

- **M3**: Enable whitehole gravity pull on entities + player, add gravity particle effects around the whitehole
- **M4**: Render a distorted perspective grid that warps toward the whitehole position (spacetime curvature effect)

Both milestones are primarily changes to [game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp) and [config.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/config.h), with a small particle system addition and grid rendering overhaul.

---

## Confirmed Design Decisions

> [!NOTE]
> **✅ Gravity on the player**: Confirmed — soft pull at 25% of entity gravity strength. Player can escape with A+direction.

> [!NOTE]
> **✅ Particle budget**: Confirmed — 8 particles, single white pixels, ~81 bytes RAM. Sufficient.

> [!NOTE]
> **✅ Grid distortion**: Confirmed — visible from the start but subtle effect. Scales with `bhCharge` over time.

> [!NOTE]
> **✅ Entity absorption**: Confirmed — entities pulled into whitehole core (< 4px) are despawned/consumed.

> [!NOTE]
> **✅ Particle visual style**: Confirmed — single white pixels (cheaper, more subtle).

> [!NOTE]
> **✅ Player gravity scale**: Confirmed — 25% of entity pull (recommended value).

## Implementation Status

| Task | Status | Notes |
|------|--------|-------|
| M3.1 — Entity gravity loop | ✅ Done | Uncommented + absorption added |
| M3.2 — Player soft gravity | ✅ Done | 25% scale, inline in game.cpp |
| M3.3 — Particle system | ✅ Done | Struct + spawn/update/render |
| M3.4 — New constants | ✅ Done | config.h updated |
| M4.1 — Grid distortion | ✅ Done | Vertical rays + segmented horizontals |
| M4.2 — Distortion helper | ✅ Done | Static helper in game.cpp |
| M4.3 — Distortion constants | ✅ Done | config.h updated |
| Particle init in reset() | ✅ Done | Zeroed on game start |
| Build verification | 🔲 TODO | Run `./build.sh --build-only` |
| Emulator playtest | 🔲 TODO | Run `./build.sh` and verify checklist |
| Tuning pass | 🔲 TODO | Adjust constants after playtesting |

---

## Proposed Changes

### M3: Whitehole Gravity + Particles

---

#### M3.1 — Enable Gravity Pull on Entities

##### [MODIFY] [game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp)

**Uncomment the gravity loop** at lines 248-255 in `updatePlaying()`. Replace the TODO block with:

```cpp
// 5. Apply blackhole gravity to entities
for (uint8_t i = 0; i < MAX_ENTITIES; i++) {
    if (entities.entities[i].active) {
        applyBlackholeGravity(entities.entities[i].x, entities.entities[i].y,
                               world.bhX, world.bhY,
                               world.bhMass, world.bhCharge, dt);

        // Optional: Absorb entities that fall into the whitehole core
        if (approxDistance(entities.entities[i].x, entities.entities[i].y,
                           world.bhX, world.bhY) < INT_TO_FP32(BH_ABSORB_RADIUS)) {
            entities.despawn(i);
        }
    }
}
```

**Key details for agents**:
- `applyBlackholeGravity()` is already fully implemented in [physics.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/physics.cpp) (lines 91-120). It uses linear falloff within the charge radius.
- The `BH_ABSORB_RADIUS` constant (= 4 pixels, defined below) is the distance at which an entity is considered "swallowed".
- This loop runs **after** `world.update()` and **before** collision checks, so entities shift toward the whitehole before the player can collide with them.

---

#### M3.2 — Enable Soft Gravity Pull on Player

##### [MODIFY] [game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp)

Add a new gravity pull on the **player** after the entity gravity loop (after step 5, before step 6). This uses a separate, weaker function call:

```cpp
// 5b. Apply soft gravity pull on the player (weaker than on entities)
{
    fp32_t dist = approxDistance(player.x, player.y, world.bhX, world.bhY);
    if (dist < world.bhCharge && dist > INT_TO_FP32(BH_ABSORB_RADIUS)) {
        fp32_t pullFactor = world.bhCharge - dist;
        fp32_t dx = world.bhX - player.x;
        fp32_t dy = world.bhY - player.y;
        fp_t effMass = FP_MUL(FP_MUL(world.bhMass, BH_PLAYER_GRAVITY_SCALE), dt);
        fp32_t moveX = (dx * (fp32_t)effMass) / dist;
        fp32_t moveY = (dy * (fp32_t)effMass) / dist;
        moveX = (moveX * pullFactor) / world.bhCharge;
        moveY = (moveY * pullFactor) / world.bhCharge;
        player.x += moveX;
        player.y += moveY;
    }
}
```

**Key details for agents**:
- `BH_PLAYER_GRAVITY_SCALE` is a new Q8.8 constant (see config.h changes below), set to `FLOAT_TO_FP(0.25)` — the player receives only 25% of the gravity force that entities do.
- This creates a subtle "tug" sensation when the player is near the whitehole, increasing tension without making the game unplayable.
- The player can always escape by holding A + direction away from the whitehole.
- The `> INT_TO_FP32(BH_ABSORB_RADIUS)` guard prevents snapping to center on overlap.

---

#### M3.3 — Add Gravity Particle System

##### [MODIFY] [game.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.h)

Add a particle struct and array as **private members** of the `Game` class:

```cpp
// ---- Whitehole accretion particles (M3) ----
struct GravityParticle {
    fp32_t x;       // World-space X (Q24.8)
    fp32_t y;       // World-space Y (Q24.8)
    uint8_t life;   // Frames remaining (0 = inactive)
    uint8_t angle;  // Current orbital angle (0-255, for respawn positioning)
};
static const uint8_t MAX_PARTICLES = 8;
GravityParticle particles[MAX_PARTICLES];
uint8_t particleSpawnTimer;  // Frames until next particle spawn

// ---- Particle helpers ----
void updateParticles(fp_t dt);
void renderParticles(HalRenderer& renderer);
void spawnParticle();
```

**RAM cost**: Each `GravityParticle` = 4 + 4 + 1 + 1 = 10 bytes. 8 particles × 10 = 80 bytes. Plus `particleSpawnTimer` (1 byte) = **81 bytes total**.

Updated memory budget: ~258 (current) + 81 = ~339 bytes. Still well within the 2560 byte SRAM budget.

##### [MODIFY] [game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp)

**Implement `spawnParticle()`**:

```cpp
void Game::spawnParticle() {
    for (uint8_t i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life == 0) {
            // Spawn at random angle on the whitehole's charge radius perimeter
            uint8_t angle = (uint8_t)(world.nextRandom() & 255);
            int16_t chargePixels = FP_TO_INT(world.bhCharge);
            if (chargePixels < 20) chargePixels = 20;  // Minimum visual radius

            // Use 16-point unit circle lookup for position on perimeter
            uint8_t dir = (angle >> 4) & 15;  // Map 0-255 to 0-15 index
            int16_t r = chargePixels + (int16_t)(world.randomRange(0, 10));
            int16_t dx = (int16_t)(((int32_t)UNIT_CIRCLE_X[dir] * r) / 127);
            int16_t dy = (int16_t)(((int32_t)UNIT_CIRCLE_Y[dir] * r) / 127);

            particles[i].x = world.bhX + INT_TO_FP32(dx);
            particles[i].y = world.bhY + INT_TO_FP32(dy);
            particles[i].life = PARTICLE_LIFETIME;
            particles[i].angle = angle;
            return;
        }
    }
}
```

**Implement `updateParticles()`**:

```cpp
void Game::updateParticles(fp_t dt) {
    // Spawn new particles periodically
    particleSpawnTimer++;
    if (particleSpawnTimer >= PARTICLE_SPAWN_INTERVAL) {
        particleSpawnTimer = 0;
        spawnParticle();
    }

    // Update existing particles: pull toward whitehole center + slight orbital drift
    for (uint8_t i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life == 0) continue;

        // Strong inward pull (particles spiral inward faster than entities)
        applyBlackholeGravity(particles[i].x, particles[i].y,
                               world.bhX, world.bhY,
                               FP_MUL(world.bhMass, PARTICLE_GRAVITY_MULT), world.bhCharge, dt);

        // Orbital tangential drift (perpendicular to radial direction)
        fp32_t dx = particles[i].x - world.bhX;
        fp32_t dy = particles[i].y - world.bhY;
        // Tangent vector: (-dy, dx) normalized and scaled
        fp32_t dist = approxDistance(particles[i].x, particles[i].y, world.bhX, world.bhY);
        if (dist > FP_ONE) {
            fp32_t tangentX = (-dy * (fp32_t)PARTICLE_ORBITAL_SPEED) / dist;
            fp32_t tangentY = (dx * (fp32_t)PARTICLE_ORBITAL_SPEED) / dist;
            particles[i].x += FP32_MUL(tangentX, dt);
            particles[i].y += FP32_MUL(tangentY, dt);
        }

        // Decrement lifetime
        particles[i].life--;

        // Also kill if very close to center (absorbed)
        if (dist < INT_TO_FP32(3)) {
            particles[i].life = 0;
        }
    }
}
```

**Implement `renderParticles()`**:

```cpp
void Game::renderParticles(HalRenderer& renderer) {
    for (uint8_t i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life == 0) continue;

        int16_t sx = world.worldToScreenX(particles[i].x, particles[i].y);
        int16_t sy = world.worldToScreenY(particles[i].y);

        // Skip if off-screen
        if (sx < 0 || sx >= SCREEN_W || sy < 0 || sy >= SCREEN_H) continue;

        // Flicker effect: draw every other frame for ethereal look
        if ((particles[i].life + i) % 2 == 0) {
            renderer.drawPixel(sx, sy, COLOR_WHITE);
        }
    }
}
```

**Call sites in `updatePlaying()` and `renderPlaying()`**:

In `updatePlaying()`, add after the entity gravity loop (after step 5b):
```cpp
// 5c. Update accretion particles
updateParticles(dt);
```

In `renderPlaying()`, add the particles to the draw order (between `renderBlackhole` and `renderPlayer`):
```cpp
void Game::renderPlaying(HalRenderer& renderer) {
    renderBackground(renderer);
    renderEntities(renderer);
    renderBlackhole(renderer);
    renderParticles(renderer);  // M3: accretion disk particles
    renderPlayer(renderer);
    renderHUD(renderer);
}
```

**Initialize particles in `reset()`**:
```cpp
// In Game::reset(), add:
for (uint8_t i = 0; i < MAX_PARTICLES; i++) {
    particles[i].life = 0;
}
particleSpawnTimer = 0;
```

---

#### M3.4 — New Constants

##### [MODIFY] [config.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/config.h)

Add after the existing `BH_RENDER_RADIUS` / `BH_START_DISTANCE` constants (~line 288):

```cpp
// =============================================================================
// BLACKHOLE GRAVITY CONSTANTS (M3)
// =============================================================================
// BH_ABSORB_RADIUS: Distance (pixels) at which entities touching the whitehole
// core are consumed/despawned. Creates a visual "absorption" effect.
static const uint8_t BH_ABSORB_RADIUS = 4;

// BH_PLAYER_GRAVITY_SCALE: Fraction of gravity force applied to the player
// compared to entities. 0.25 = player feels 25% of the pull. Low enough to
// be escapable with A + direction, high enough to create tension.
static const fp_t BH_PLAYER_GRAVITY_SCALE = FLOAT_TO_FP(0.25);

// =============================================================================
// ACCRETION PARTICLE CONSTANTS (M3)
// =============================================================================
// Lightweight particle system for the whitehole accretion disk visual effect.
// Particles spawn on the charge radius perimeter and spiral inward with orbital
// tangential drift, creating a swirling visual aura.
static const uint8_t PARTICLE_LIFETIME = 90;         // Frames before particle fades (~1.5 sec)
static const uint8_t PARTICLE_SPAWN_INTERVAL = 8;    // Frames between new particle spawns
static const fp_t PARTICLE_GRAVITY_MULT = FLOAT_TO_FP(3.0);  // Particles pulled 3x stronger than entities
static const fp_t PARTICLE_ORBITAL_SPEED = FLOAT_TO_FP(0.5);  // Tangential orbital speed (px/frame)
```

---

### M4: Distorted Perspective Grid (Spacetime Curvature)

---

#### M4.1 — Grid Ray Distortion Toward Whitehole

##### [MODIFY] [game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp) — `renderBackground()`

Replace the current `renderBackground()` method (lines 358-394) with an enhanced version that distorts perspective rays toward the whitehole's screen-space position:

**Algorithm overview**:
1. Compute the whitehole's screen position: `bhSX`, `bhSY`.
2. Compute a distortion radius in screen pixels: `distortRadius = min(FP_TO_INT(bhCharge) / 2, GRID_DISTORT_MAX_RADIUS)`.
3. For each vertical perspective ray and horizontal depth line, compute the endpoint normally, then **bend it toward the whitehole** proportionally to how close it is to the whitehole position. Rays/lines closer to the whitehole bend more, creating a "gravity lens" effect.

**Detailed implementation**:

```cpp
void Game::renderBackground(HalRenderer& renderer) {
    // 1. Horizon line (only drawn if vanishing point is on-screen)
    if (HORIZON_Y >= 0) {
        renderer.drawLine(0, HORIZON_Y, SCREEN_W - 1, HORIZON_Y, COLOR_WHITE);
    }

    // 2. Compute whitehole screen position for grid distortion
    int16_t bhSX = world.worldToScreenX(world.bhX, world.bhY);
    int16_t bhSY = world.worldToScreenY(world.bhY);

    // Distortion strength scales with bhCharge (grows over time)
    int16_t distortStrength = (int16_t)(FP_TO_INT(world.bhCharge) / GRID_DISTORT_DIVISOR);
    if (distortStrength > GRID_DISTORT_MAX_STRENGTH) {
        distortStrength = GRID_DISTORT_MAX_STRENGTH;
    }
    int16_t distortRadiusSq = (int16_t)GRID_DISTORT_RADIUS * GRID_DISTORT_RADIUS;

    // 3. Perspective rays converging toward vanishing point (scrolling with camX)
    int16_t camX_int = (int16_t)FP32_TO_INT(world.camX);
    int16_t xOffset = camX_int % BASE_SPACING_X;
    if (xOffset < 0) xOffset += BASE_SPACING_X;

    for (int16_t bx = -xOffset - BASE_SPACING_X * 2;
         bx <= SCREEN_W + BASE_SPACING_X * 2;
         bx += BASE_SPACING_X) {

        int16_t tx = (SCREEN_W / 2) + ((bx - (SCREEN_W / 2)) * TOP_SPACING_X) / BASE_SPACING_X;

        // Apply grid distortion: bend top and bottom endpoints toward whitehole
        int16_t txD = applyGridDistortion(tx, HORIZON_Y, bhSX, bhSY, distortStrength, distortRadiusSq);
        int16_t bxD = applyGridDistortion(bx, SCREEN_H - 1, bhSX, bhSY, distortStrength, distortRadiusSq);

        renderer.drawLine(txD, HORIZON_Y, bxD, SCREEN_H - 1, COLOR_WHITE);
    }

    // 4. Quadratic foreshortened depth lines (scrolling with camY) with distortion
    int16_t camY_int = (int16_t)FP32_TO_INT(world.camY);
    int16_t zOffset = camY_int % Z_PERIOD;
    if (zOffset < 0) zOffset += Z_PERIOD;

    for (int16_t z = Z_PERIOD - zOffset; z <= PERSPECTIVE_MAX_Z; z += Z_PERIOD) {
        int32_t z32 = z;
        int16_t yLine = HORIZON_Y + (int16_t)(((int32_t)GROUND_HEIGHT * z32 * z32) / PERSPECTIVE_MAX_Z_SQ);
        if (yLine <= 0 || yLine >= SCREEN_H) continue;

        // Draw distorted horizontal line as segmented polyline
        // (distortion bends segments toward whitehole)
        int16_t prevX = 0;
        int16_t prevY = yLine;
        static const uint8_t HSEG_COUNT = 8;  // 8 segments across screen width
        int16_t segW = SCREEN_W / HSEG_COUNT;

        for (uint8_t s = 1; s <= HSEG_COUNT; s++) {
            int16_t cx = (s == HSEG_COUNT) ? (SCREEN_W - 1) : (s * segW);
            int16_t cy = yLine;

            // Distort this control point
            int16_t dxBh = cx - bhSX;
            int16_t dyBh = cy - bhSY;
            int32_t d2 = (int32_t)dxBh * dxBh + (int32_t)dyBh * dyBh;

            if (d2 > 0 && d2 < (int32_t)distortRadiusSq) {
                // Linear falloff: stronger distortion closer to whitehole
                // shift = distortStrength * (1 - d/maxD) toward whitehole
                // Approximate: shift proportional to (distortRadiusSq - d2) / distortRadiusSq
                int16_t shiftX = (int16_t)(((int32_t)-dxBh * distortStrength * (distortRadiusSq - d2)) /
                                           ((int32_t)distortRadiusSq * 64));
                int16_t shiftY = (int16_t)(((int32_t)-dyBh * distortStrength * (distortRadiusSq - d2)) /
                                           ((int32_t)distortRadiusSq * 64));
                cx += shiftX;
                cy += shiftY;
            }

            renderer.drawLine(prevX, prevY, cx, cy, COLOR_WHITE);
            prevX = cx;
            prevY = cy;
        }
    }
}
```

**Key design decisions**:
- Vertical perspective rays: only the **X endpoints** are distorted (top and bottom), keeping the rays as straight lines but with shifted positions, creating a "gravitational lens" effect with zero extra RAM.
- Horizontal depth lines: drawn as **segmented polylines** (8 segments) instead of single lines. Each segment control point is shifted toward the whitehole based on distance. This creates the visible curvature.
- Segment count (8) is a balance between visual quality and CPU cost. Each horizontal line becomes 8 `drawLine()` calls instead of 1. With ~5 visible depth lines, that's ~40 line draws vs ~5.

---

#### M4.2 — Grid Distortion Helper Function

##### [MODIFY] [game.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.h)

Add a private helper method:

```cpp
// ---- Grid distortion helper (M4) ----
// Returns a distorted X coordinate for a grid point at (px, py),
// pulled toward the whitehole at (bhSX, bhSY) with given strength.
static int16_t applyGridDistortion(int16_t px, int16_t py,
                                    int16_t bhSX, int16_t bhSY,
                                    int16_t strength, int16_t radiusSq);
```

##### [MODIFY] [game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp)

Implement:

```cpp
int16_t Game::applyGridDistortion(int16_t px, int16_t py,
                                   int16_t bhSX, int16_t bhSY,
                                   int16_t strength, int16_t radiusSq) {
    int16_t dx = px - bhSX;
    int16_t dy = py - bhSY;
    int32_t d2 = (int32_t)dx * dx + (int32_t)dy * dy;

    if (d2 <= 0 || d2 >= (int32_t)radiusSq) {
        return px;  // Outside distortion radius — no effect
    }

    // Shift X toward whitehole, proportional to (radiusSq - d2) / radiusSq
    int16_t shiftX = (int16_t)(((int32_t)-dx * strength * (radiusSq - d2)) /
                               ((int32_t)radiusSq * 64));
    return px + shiftX;
}
```

**Why only shift X**: The perspective grid's horizontal depth lines handle Y distortion via the segmented polyline approach. For vertical rays, shifting only X creates the classic "gravitational lensing" appearance where rays bend laterally around the massive object. This is both visually correct and computationally cheaper.

---

#### M4.3 — Grid Distortion Constants

##### [MODIFY] [config.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/config.h)

Add after the particle constants:

```cpp
// =============================================================================
// GRID DISTORTION CONSTANTS (M4 — SPACETIME CURVATURE)
// =============================================================================
// The perspective ground grid warps toward the whitehole, creating a visual
// "spacetime curvature" effect. Distortion strength scales with bhCharge
// (grows over time), so the effect becomes more dramatic as the game progresses.
static const uint8_t GRID_DISTORT_DIVISOR = 4;        // bhCharge / this = distortion strength
static const uint8_t GRID_DISTORT_MAX_STRENGTH = 12;  // Cap on distortion intensity (pixels)
static const uint8_t GRID_DISTORT_RADIUS = 80;        // Screen-space radius of distortion effect (px)
```

---

## Verification Plan

### Automated Tests

```bash
# Compile the full Arduboy sketch to check for syntax errors, type mismatches, and flash/SRAM overflow
./build.sh --build-only

# Verify SRAM usage is within budget (should be < 2200 bytes, leaving ~360 for stack)
# The build output will show "Global variables use X bytes (Y%) of dynamic memory"
```

### Manual Verification

1. **Build and run in ProjectABE emulator**:
   ```bash
   ./build.sh
   ```
   Open `http://localhost:8000/` in browser to test in the web-based Arduboy emulator.

2. **M3 — Gravity verification checklist**:
   - [ ] Entities near the whitehole visibly drift toward it
   - [ ] Entities far from the whitehole (outside charge radius) are unaffected
   - [ ] Entities pulled into the whitehole core (< 4px) are despawned
   - [ ] Player feels a subtle tug when inside the charge radius
   - [ ] Player can still escape the pull by holding A + direction away
   - [ ] Game over still triggers correctly when whitehole overlaps player
   - [ ] Difficulty ramp still works (bhSpeed and bhCharge increase over time)

3. **M3 — Particle verification checklist**:
   - [ ] White pixels appear around the whitehole and spiral inward
   - [ ] Particles spawn on the charge radius perimeter
   - [ ] Particles fade after ~1.5 seconds
   - [ ] Particles flicker (drawn every other frame) for ethereal look
   - [ ] No visual glitches when whitehole is partially or fully off-screen
   - [ ] Frame rate stays at 60 FPS (particles are cheap single-pixel draws)

4. **M4 — Distorted grid verification checklist**:
   - [ ] Grid lines visibly bend toward the whitehole position
   - [ ] Distortion is stronger closer to the whitehole
   - [ ] Distortion grows over time as bhCharge increases
   - [ ] Grid lines outside the distortion radius remain straight
   - [ ] Horizontal depth lines show smooth curvature (not jagged jumps)
   - [ ] Grid scrolling still works correctly with camera movement
   - [ ] No visual artifacts when the whitehole is off-screen
   - [ ] Frame rate stays at 60 FPS

5. **Integration test**: Play a full game session (30+ seconds) and verify:
   - [ ] Spawned items near the whitehole orbit get pulled in over time
   - [ ] The risk-reward money distribution still functions (items spawn safely outside charge radius)
   - [ ] The grid distortion creates a dramatic "spacetime curvature" visual
   - [ ] Overall game feel is improved — the whitehole feels more dangerous and alive
