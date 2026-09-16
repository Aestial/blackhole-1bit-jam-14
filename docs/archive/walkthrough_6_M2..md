# Milestone 2 (M2) Walkthrough — Entity Spawning, Object Pool, Collision & Power-Ups

## Summary of Completed Work
Milestone 2 (M2) has been fully implemented and verified in the emulator. The game now features:
1. **Zero-Allocation Static Object Pool**: An embedded-optimized pool (`MAX_ENTITIES = 12`) managing active and inactive entities with $O(1)$ despawning and zero heap fragmentation.
2. **Dynamic Spawning System**: Items spawn across the pseudo-3D perspective grid with natural distribution, travel-direction bias, and initial scatter at game start.
3. **Collision Detection & Behavior**:
   - **Collectibles (Diamond Gems)**: Awards +10 points with a streak combo multiplier ($1\times \to 2\times \to 3\times \to 4\times$) for consecutive pickups.
   - **Power-Ups (Coffee Mug)**: Grants $+35\%$ speed boost and $+50\%$ acceleration for 3 seconds (180 frames), cleanses active food slows, and adds speed trail particles.
   - **Food Hazards (Pizza, Burger, Donut, Ice Cream)**: Contact causes speed penalties ($-15\%$ to $-60\%$), breaks active score streaks, and causes character debuff blinking.
4. **Despawning**: Cleans up entities that move beyond the screen bounds or $>140$ world units away, recycling slots for future spawns.
5. **HUD & Visual Feedback**: Enhanced score overlay with sleek border box, live combo multiplier display, and boost indicators.

---

## Gameplay Demonstration

![Initial Items on Perspective Grid](file:///home/dorito/.gemini/antigravity-ide/brain/9e04b54b-8891-4396-b00f-9846df8b76cc/m2_initial_items_on_grid_1789599881264.png)

*The pseudo-3D plane with the player centered, Whitehole pursuing from the left, a Pizza hazard in the upper-left, a Diamond gem in the upper-right, and a Coffee power-up in the lower-right foreground.*

---

## Technical Changes by Component

### 1. Configuration & Constants ([config.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/config.h))
- Configured hitboxes for fair arcade play: 8px for food obstacles (forgiving near-misses) and 10px for collectibles & coffee (responsive pickup feel).
- Added `POWERUP_COFFEE_DURATION = 180`, `POWERUP_COFFEE_SPEED_BOOST = FLOAT_TO_FP(1.35)`, and `POWERUP_COFFEE_ACCEL_BOOST = FLOAT_TO_FP(1.50)`.
- Added `COMBO_MAX = 4` for streak multipliers.

### 2. Entity Model & Object Pool ([entity.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/entity.h), [entity.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/entity.cpp))
- Added `ENTITY_POWERUP_COFFEE` to `EntityType` enum.
- Added `isPowerup()` helper method to `Entity`.
- Updated `EntityManager::spawn()` to support power-up initialization and proper hitbox dimensions.

### 3. Player Physics & Power-Up State ([player.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/player.h), [player.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/player.cpp))
- Added `boostTimer` and `boostTimerAccum` fields.
- Implemented `applyCoffeeBoost()`: applies turbo speed and acceleration, while instantly cleansing any active food slow debuff.
- In `Player::update()`: applied boost multiplier when `boostTimer > 0` and decremented frame timers.
- Added `isBoosted()` status check.

### 4. World & Spawning Engine ([world.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/world.h), [world.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/world.cpp))
- Added a deterministic 16-bit Galois LFSR pseudo-random number generator (`nextRandom()`, `randomRange()`) for platform-independent spawning.
- Updated `isTooFar()` to combine world-space radial distance ($>140$ units) and screen bounds for despawning.

### 5. Game Orchestrator & Rendering ([game.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.h), [game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp))
- Added `comboMultiplier` to game state.
- Pre-spawned 3 initial items in `Game::reset()` so the game feels active from frame 1.
- Implemented full item spawning pipeline in `updatePlaying()` with type weighting:
  - $35\%$ Diamond Collectibles
  - $15\%$ Coffee Power-Ups
  - $50\%$ Food Hazards
- Implemented AABB collision checks with `checkOverlap()`:
  - Applying slow debuffs and resetting combo on food.
  - Adding combo-multiplied score on diamond collection.
  - Applying speed boost and cleansing on coffee pickup.
- Updated `renderEntities()` to draw `SPRITE_ITEM_COFFEE`.
- Updated `renderPlayer()` to render motion trail particles when boosted.
- Updated `renderHUD()` with sleek dimensions (`HUD_BG_W = 42`) and combo/boost text.

### 6. Documentation ([gdd.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/gdd.md), [architecture.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/architecture.md))
- Updated Milestone 2 status to Complete across all architectural and game design specifications.

---

## Verification & Resource Budget

### Hardware Limits (Arduboy / ATmega32u4)
| Resource | Used | Maximum | Percentage | Headroom |
|---|---|---|---|---|
| **Program Storage (Flash)** | 17,202 bytes | 28,672 bytes | 59.9% | 11,470 bytes |
| **Dynamic Memory (SRAM)** | 1,483 bytes | 2,560 bytes | 57.9% | 1,077 bytes |

### Visual & Functional Checks
- [x] AVR compilation passes cleanly with zero warnings/errors.
- [x] Initial entities spawn at game start and are visible on the perspective grid.
- [x] Entities naturally converge toward the vanishing point with perspective foreshortening.
- [x] Collision responses work for food, collectibles, and coffee power-ups.
- [x] Out-of-bounds entities despawn cleanly and slots are recycled.
- [x] HUD displays score and combo multipliers without cluttering the screen.
