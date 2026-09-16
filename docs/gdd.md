# Supermassive Whitehole — Game Design Document

## Overview

| | |
|---|---|
| **Title** | Supermassive Whitehole |
| **Platform** | Arduboy (128×64, 1-bit, ATmega32u4) |
| **Jam** | [1-Bit Jam 14](https://itch.io/jam/1-bit-jam-14) |
| **Genre** | Top-down arcade survival / score attack |
| **Controls** | D-pad (8-dir) + A (accelerate) + B (brake) |

A fat man runs on a pseudo-3D top-down infinite plane, dodging delicious food (Pizza, Burger, Donut, and brain-freezing Ice Cream) and fleeing a pursuing supermassive whitehole. Inspired by the car control in [Crates](https://jessemillar.itch.io/crates).

---

## Core Mechanics

### Hybrid Inertia Controls
- D-pad sets desired direction (8 directions + idle)
- A button applies thrust in desired direction
- B button brakes (increases friction)
- Strong inertia: the fat man drifts and slides when changing direction
- Feels like steering a heavy object — satisfying and skillful

### Blackhole Entity
Properties translated from real black hole physics:
- **Mass**: Gravitational pull strength on nearby entities (constant)
- **Spin**: Visual rotation effect (cosmetic only)
- **Charge**: Attraction radius — how far gravity reaches (increases over time)

The blackhole tracks the player's position at a speed that linearly increases over time.

### Food Obstacles
| Type | Shape | Slow Duration | Slow Intensity |
|------|-------|---------------|----------------|
| Pizza | Triangle | 0.5 sec | −15% speed |
| Burger | Square | 1.0 sec | −30% speed |
| Donut | Circle | 1.5 sec | −50% speed |

### Collectibles & Scoring
- Diamond-shaped collectibles spawn on the plane (+10 points each)
- Passive score: +1 point per second survived
- High score persisted to EEPROM

### Infinite Plane
- Camera follows player
- Square line grid background with lines converging toward blackhole
- Entities spawn around the player, despawn when far off-screen

---

## Game States

```
TITLE → (Press A) → PLAYING → (Blackhole catches player) → GAME OVER
                                                              ↓ A → TITLE
                                                              ↓ B → PLAYING (retry)
```

---

## Difficulty Curve (Linear)
- Blackhole speed: increases by 1/256 per frame
- Blackhole charge (gravity radius): increases by 1/256 per frame
- Spawn interval: decreases by 1 frame every 2 seconds, min 20 frames

---

## Technical Constraints
- **RAM**: 2.5 KB total — static allocation only, no `new`/`malloc`
- **Flash**: 32 KB — code + PROGMEM data
- **Math**: Q8.8 fixed-point (no floats)
- **Entities**: Max 12 on-screen (168 bytes)
- **Display**: 128×64 monochrome OLED
- **Frame rate**: 60 FPS target

---

## Art Style
1-bit (black and white) using dedicated PROGMEM bitmap sprites:
- **Player**: 16x16 pixel character (`player_static.png`) with slow-debuff blinking
- **Whitehole**: 32x32 animated swirling celestial hazard (4 frames, `whitehole_32.png`)
- **Food**: 16x16 sprites (`items_16.png`):
  - Pizza (light slow, -15%)
  - Burger (medium slow, -30%)
  - Donut (heavy slow, -50%)
  - Ice Cream (brain freeze, -60%)
- **Collectible**: 16x16 Gem / Crystal (`items_16.png`, +score)

---

## Milestones
| Phase | Deliverable |
|-------|-------------|
| M0 | Project scaffold, architecture, HAL interfaces |
| M1 | Player movement with inertia physics, scrolling grid |
| M2 | Entity spawning, collision detection, scoring |
| M3 | Blackhole chase, gravity pull, difficulty ramp |
| M4 | Polished UI (title, game over, HUD), converging grid |
| M5 | Real sprites, visual effects, audio |
| M6 | Playtesting, balance tuning, jam submission |
