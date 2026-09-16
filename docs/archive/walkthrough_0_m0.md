# M0 Walkthrough — Project Scaffold Complete

## What Was Built

Milestone M0 scaffolded the entire project architecture for **Supermassive Blackhole** — 20 new/modified files across 4 layers, compiling cleanly on Arduboy.

## Build Result ✅

```
Sketch uses 10370 bytes (36%) of program storage space. Maximum is 28672 bytes.
Global variables use 1475 bytes (57%) of dynamic memory, leaving 1085 bytes for local variables.
```

Plenty of headroom for M1–M5 features.

## Directory Structure

```
blackhole/
├── blackhole.ino                              ← Entry point (wires HAL → Game)
├── src/
│   ├── game/                                  ← Pure C++ game logic (no platform deps)
│   │   ├── config.h                           ← Constants, fixed-point math, physics tuning
│   │   ├── entity.h / entity.cpp              ← Entity + EntityManager (COMPLETE)
│   │   ├── player.h / player.cpp              ← Player with inertia docs (basic stub)
│   │   ├── physics.h / physics.cpp            ← Collision + gravity (COMPLETE)
│   │   ├── world.h / world.cpp                ← Camera, blackhole, spawning (COMPLETE)
│   │   └── game.h / game.cpp                  ← State machine + rendering (stubs + TODOs)
│   ├── hal/                                   ← HAL interface contracts (documentation)
│   │   ├── renderer.h, input.h, storage.h     ← API specs for platform implementers
│   │   └── hal_types.h                        ← #ifdef platform selector (typedefs)
│   └── platform/arduboy/                      ← Arduboy2 HAL implementations (COMPLETE)
│       ├── arduboy_renderer.h                 ← Wraps Arduboy2 draw calls
│       ├── arduboy_input.h                    ← Wraps button input
│       └── arduboy_storage.h                  ← Wraps EEPROM
├── assets/
│   └── sprites.h                              ← Placeholder shape constants
└── docs/
    ├── gdd.md                                 ← Game design document
    ├── architecture.md                        ← Architecture & porting guide
    └── implementation_plan_m0_archive.md       ← Original design plan (archived)
```

## Implementation Status by Component

| Component | Status | Notes |
|-----------|--------|-------|
| **config.h** | ✅ Complete | All constants, fixed-point macros, Q8.8/Q24.8 types |
| **Entity / EntityManager** | ✅ Complete | `spawn()`, `despawn()`, `reset()`, `countActive()` all implemented |
| **Player** | 🟡 Basic stub | `init()`, `applyFoodSlow()` complete. `update()` has basic movement (no inertia blending yet — TODO M1) |
| **Physics** | ✅ Complete | `checkOverlap()`, `approxDistance()`, `moveToward()`, `applyBlackholeGravity()` all implemented |
| **World** | ✅ Complete | Camera tracking, blackhole movement, difficulty ramp, spawn/score timers, coordinate transforms |
| **Game** | 🟡 Functional stubs | State machine works, title/gameover screens show text, basic player circle + dot-grid render. Entity rendering, spawning, and collision TODO'd for M2-M3 |
| **HAL (Arduboy)** | ✅ Complete | All 3 HAL implementations (renderer, input, storage) are done |
| **Docs** | ✅ Complete | GDD + architecture/porting guide |

## Key Architecture Decisions

1. **Only `game.h/cpp` touches HAL types** — all other game files are 100% platform-independent
2. **Compile-time binding via `#ifdef`** — zero overhead, no virtual dispatch
3. **To port**: add `src/platform/<name>/`, update `hal_types.h`, change `#define` in `config.h`
4. **Every TODO is tagged** with its milestone: `TODO(M1)`, `TODO(M2)`, etc.

## What a Running Game Looks Like Now

- **Title screen**: Shows "SUPERMASSIVE BLACKHOLE", high score, "Press A"
- **Playing**: Fat man (white circle) at center, scrolling dot grid, blackhole (concentric circles) chasing, score counting up
- **Game over**: Shows final score, high score, A/B retry prompts
- State transitions all work (Title → A → Playing → caught → GameOver → A/B)

## Next Steps (for other agents)

Find all TODOs: `grep -rn "TODO(M" src/`

| Milestone | Key Files | What to Do |
|-----------|-----------|------------|
| **M1** | `player.cpp`, `game.cpp` | Implement full inertia blending in `player.update()`, upgrade background to line grid |
| **M2** | `game.cpp` | Uncomment+implement entity spawning, collision responses, `renderEntities()` |
| **M3** | `game.cpp` | Uncomment blackhole gravity loop, improve `renderBlackhole()` |
| **M4** | `game.cpp` | Polish all render methods, add converging grid distortion |
| **M5** | `assets/sprites.h`, `game.cpp` | Replace primitives with PROGMEM bitmap sprites |
