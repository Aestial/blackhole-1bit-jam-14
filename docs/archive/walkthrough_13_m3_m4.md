# M3–M4 Walkthrough: Gravity, Particles & Distorted Grid

## Build Status

✅ **Compilation passed** — no errors, no warnings.

| Resource | Usage | Budget | Margin |
|----------|-------|--------|--------|
| **Flash** | 23,246 bytes (81%) | 28,672 bytes | 5,426 bytes remaining |
| **SRAM** | 1,599 bytes (62%) | 2,560 bytes | 961 bytes for stack |

---

## Changes Made

### Files Modified

| File | Lines Changed | Purpose |
|------|--------------|---------|
| [config.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/config.h) | +37 lines | New constants for gravity, particles, grid distortion |
| [game.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.h) | +43 lines | GravityParticle struct, particle array, new method declarations |
| [game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp) | ~+180 lines | All M3/M4 implementations |

---

### M3: Whitehole Gravity + Particles

#### Entity Gravity (M3.1) — [game.cpp L249-262](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp#L249-L262)
- Uncommented the gravity loop — entities within `bhCharge` radius are now pulled toward the whitehole
- Added entity absorption: entities within `BH_ABSORB_RADIUS` (4px) of center are despawned

#### Player Gravity (M3.2) — [game.cpp L264-278](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp#L264-L278)
- Soft gravity pull at 25% of entity strength (`BH_PLAYER_GRAVITY_SCALE = 0.25`)
- Uses same linear falloff as entity gravity — stronger closer to center
- Player can always escape with A + direction

#### Particle System (M3.3) — [game.cpp L670-795](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp#L670-L795)
- **`spawnParticle()`**: Spawns on charge radius perimeter using unit circle lookup table
- **`updateParticles()`**: Strong inward pull (3x entity gravity) + orbital tangential drift
- **`renderParticles()`**: Single white pixels with flicker effect (draw every other frame)
- Particles are initialized to dead in `reset()` and updated in `updatePlaying()`
- Rendered between `renderBlackhole()` and `renderPlayer()` in painter's algorithm

#### New Constants (M3.4) — [config.h L289-317](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/config.h#L289-L317)
- `BH_ABSORB_RADIUS = 4` — absorption distance
- `BH_PLAYER_GRAVITY_SCALE = 0.25` — player feels 25% gravity
- `MAX_GRAVITY_PARTICLES = 8` — particle pool size
- `PARTICLE_LIFETIME = 90` — ~1.5 seconds
- `PARTICLE_SPAWN_INTERVAL = 8` — new particle every 8 frames
- `PARTICLE_GRAVITY_MULT = 3.0` — particles pulled 3x harder
- `PARTICLE_ORBITAL_SPEED = 0.5` — tangential drift speed

### M4: Distorted Perspective Grid

#### Grid Distortion (M4.1) — [game.cpp L355-449](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp#L355-L449)
- Vertical perspective rays: endpoints bent toward whitehole via `applyGridDistortion()`
- Horizontal depth lines: drawn as 8-segment polylines with per-segment distortion
- Each segment control point shifts toward whitehole with linear distance falloff
- Distortion grows over time as `bhCharge` increases

#### Distortion Helper (M4.2) — [game.cpp L797-816](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp#L797-L816)
- Pure function — returns distorted X coordinate
- Linear falloff: `shift ∝ (radiusSq - d²) / radiusSq`
- Points outside distortion radius pass through unchanged

#### Distortion Constants (M4.3) — [config.h L319-328](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/config.h#L319-L328)
- `GRID_DISTORT_DIVISOR = 4` — bhCharge/4 = initial distortion strength
- `GRID_DISTORT_MAX_STRENGTH = 12` — cap on distortion intensity
- `GRID_DISTORT_RADIUS = 80` — screen-space effect radius
- `GRID_HSEG_COUNT = 8` — segments per horizontal line

---

## Next Steps (for future agents / developers)

### 🔲 Immediate: Emulator Playtest
```bash
./build.sh
# Opens http://localhost:8000/ — play the game and verify:
```

**Gravity checklist**:
- [ ] Entities near the whitehole visibly drift toward it
- [ ] Entities far from whitehole (outside charge radius) unaffected
- [ ] Entities absorbed when reaching whitehole core (< 4px)
- [ ] Player feels subtle tug inside charge radius
- [ ] Player can escape pull with A + direction
- [ ] Game over triggers correctly on whitehole overlap

**Particle checklist**:
- [ ] White pixels appear and spiral around whitehole
- [ ] Particles flicker (drawn every other frame)
- [ ] No visual glitches when whitehole is off-screen
- [ ] 60 FPS maintained

**Grid distortion checklist**:
- [ ] Grid lines bend toward whitehole position
- [ ] Distortion stronger closer to whitehole
- [ ] Distortion grows over time with bhCharge
- [ ] Grid scrolling still works with camera movement
- [ ] No artifacts when whitehole is off-screen

### 🔲 Tuning Pass
If effects are too strong or too weak, adjust these constants in [config.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/config.h):

| Constant | Current | If too strong | If too weak |
|----------|---------|---------------|-------------|
| `BH_PLAYER_GRAVITY_SCALE` | 0.25 | Decrease to 0.15 | Increase to 0.35 |
| `PARTICLE_GRAVITY_MULT` | 3.0 | Decrease to 2.0 | Increase to 4.0 |
| `PARTICLE_ORBITAL_SPEED` | 0.5 | Decrease to 0.3 | Increase to 0.8 |
| `GRID_DISTORT_MAX_STRENGTH` | 12 | Decrease to 8 | Increase to 16 |
| `GRID_DISTORT_DIVISOR` | 4 | Increase to 6 | Decrease to 3 |

### 🔲 Remaining Milestones
- **M5**: Real sprites, 8-direction player animation, visual effects, audio
- **M6**: Playtesting, balance tuning, jam submission
- **TODO item**: Fix player controller input (still feels like a car) — see [TODO.md line 13](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/TODO.md#L13)
