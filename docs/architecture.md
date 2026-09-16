# Architecture & Porting Guide

## Overview

This project uses a **layered architecture** that separates game logic from platform-specific code. This enables porting to other platforms (Raylib, SDL2, etc.) by swapping only the Hardware Abstraction Layer (HAL) implementation.

```
┌─────────────────────────────────────────┐
│  supermassive-whitehole.ino (Entry Point)            │  ← Wires HAL to Game
├─────────────────────────────────────────┤
│  src/game/  (Game Logic — Pure C++)     │  ← Platform-independent
│    config.h, player.h/cpp, entity.h/cpp │
│    world.h/cpp, physics.h/cpp           │
│    game.h/cpp ← ONLY file using HAL     │
├─────────────────────────────────────────┤
│  src/hal/  (HAL Interfaces)             │  ← API contracts (docs)
│    renderer.h, input.h, storage.h       │
│    hal_types.h ← Platform type selector │
├─────────────────────────────────────────┤
│  src/platform/arduboy/  (Concrete HAL)  │  ← Arduboy2 wrappers
│    arduboy_renderer.h                   │
│    arduboy_input.h                      │
│    arduboy_storage.h                    │
└─────────────────────────────────────────┘
```

## File Dependency Rules

| File | Can include | Cannot include |
|------|-------------|----------------|
| `src/game/config.h` | `<stdint.h>` only | Any platform header |
| `src/game/player.h` | `config.h`, `entity.h` | Any HAL or platform header |
| `src/game/entity.h` | `config.h` | Any HAL or platform header |
| `src/game/world.h` | `config.h`, `entity.h` | Any HAL or platform header |
| `src/game/physics.h` | `config.h` | Any HAL or platform header |
| `src/game/game.h` | All game headers + `hal_types.h` | Direct platform headers |
| `src/hal/hal_types.h` | Platform headers via `#ifdef` | — |

**Key rule**: Only `game.h/cpp` includes `hal_types.h`. All other game files are 100% platform-independent.

## Compile-Time HAL Binding

We use `#ifdef` + `typedef` instead of `virtual` interfaces (zero overhead on AVR):

```cpp
// src/hal/hal_types.h
#ifdef PLATFORM_ARDUBOY
#include "../platform/arduboy/arduboy_renderer.h"
typedef ArduboyRenderer HalRenderer;
...
#elif defined(PLATFORM_RAYLIB)
#include "../platform/raylib/raylib_renderer.h"
typedef RaylibRenderer HalRenderer;
...
#endif
```

Game code uses `HalRenderer`, `HalInput`, `HalStorage` — abstract names that resolve to concrete types at compile time.

## How to Port to a New Platform

### Step 1: Create Platform Directory
```
src/platform/raylib/
  raylib_renderer.h
  raylib_input.h
  raylib_storage.h
```

### Step 2: Implement HAL Structs
Each struct must implement the methods documented in `src/hal/renderer.h`, `input.h`, `storage.h`:

```cpp
// raylib_renderer.h
struct RaylibRenderer {
    void clear() { ClearBackground(BLACK); }
    void display() { EndDrawing(); }
    void drawRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color) {
        DrawRectangleLines(x, y, w, h, color ? WHITE : BLACK);
    }
    // ... all methods from src/hal/renderer.h
};
```

### Step 3: Update hal_types.h
```cpp
#elif defined(PLATFORM_RAYLIB)
#include "../platform/raylib/raylib_renderer.h"
#include "../platform/raylib/raylib_input.h"
#include "../platform/raylib/raylib_storage.h"
typedef RaylibRenderer HalRenderer;
typedef RaylibInput    HalInput;
typedef RaylibStorage  HalStorage;
```

### Step 4: Change Platform Define
In `config.h`, change:
```cpp
#define PLATFORM_ARDUBOY  →  #define PLATFORM_RAYLIB
```

### Step 5: Create Entry Point
Replace `supermassive-whitehole.ino` with a `main.cpp`:
```cpp
#include "src/game/game.h"
int main() {
    // Init Raylib window, create HAL instances, run game loop
}
```

### Step 6: Build
Compile with your platform's toolchain (gcc, cmake, etc.).

## Fixed-Point Math (Q8.8)

All game math uses `int16_t` (Q8.8) or `int32_t` (Q24.8) fixed-point:
- `FP_SHIFT = 8`: 8 fractional bits → precision of 1/256
- `INT_TO_FP(x)`: Convert integer to fixed-point
- `FP_TO_INT(x)`: Convert fixed-point to integer (truncate)
- `FP_MUL(a, b)`: Multiply two Q8.8 values (uses 32-bit intermediate)
- `fp_t` = `int16_t`: Small values (velocity, acceleration)
- `fp32_t` = `int32_t`: Large values (world coordinates)

When porting to a platform with FPU (e.g., PC), you can replace these with `float` if preferred, but the fixed-point code works everywhere.

## Memory Budget (Arduboy)

| Component | Size |
|-----------|------|
| Player | ~30 bytes |
| World | ~50 bytes |
| EntityManager (12 entities) | ~168 bytes |
| Game (misc) | ~10 bytes |
| **Total game data** | **~258 bytes** |
| Stack headroom | ~200-300 bytes |
| Arduboy2 framebuffer | 1024 bytes |
| **Available SRAM** | **2560 bytes** |

## Build System

**Arduino IDE / Arduino CLI** (no PlatformIO):
```bash
# Compile
arduino-cli compile --fqbn arduboy:avr:arduboy ./

# Upload
arduino-cli upload --fqbn arduboy:avr:arduboy --port /dev/ttyACM0 ./
```

## Milestones & Roadmap

| Milestone | Status | Files Modified | Description |
|-----------|--------|----------------|-------------|
| **M0** | ✅ Complete | Full tree | Project scaffold, architecture, HAL interfaces, game state machine |
| **M1** | ✅ Complete | `player.cpp`, `game.cpp`, `config.h`, `world.cpp` | Hybrid inertia physics, full-screen pseudo-3D ground grid (vanishing point above screen, no visible horizon), configurable HUD position |
| **M2** | 📋 Planned | `game.cpp`, `world.cpp` | Entity spawning (random type+position), collision responses, despawning, `renderEntities()` |
| **M3** | 📋 Planned | `game.cpp` | Enable blackhole gravity loop, `renderBlackhole()` with concentric rings + spin |
| **M4** | 📋 Planned | `game.cpp` | Polish `renderTitle()`, `renderGameOver()`, `renderHUD()`, converging grid distortion |
| **M5** | 📋 Planned | `assets/sprites.h`, `game.cpp` | Replace primitives with PROGMEM bitmap sprites, 8-dir player animation |
| **M6** | 📋 Planned | All files | Playtesting, balance tuning, bug fixes |
