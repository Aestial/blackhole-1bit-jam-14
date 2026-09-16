# Milestone 2 (M2) Update — Spacious Items, Diamond & Bills, and 8 Food Hazards

## Summary of Completed Work
In response to gameplay feeling and pacing feedback at high speeds, we have refined the entity spawning, visuals, and hazard diversity:
1. **Spacious & Distinct Layout**:
   - Spawns now enforce a minimum separation rule (`MIN_ITEM_SEPARATION = 45` world units) between all active entities, preventing clustering and guaranteeing clear runways.
   - Reduced starting items from 3 to 2 far-apart items (>60px separation) so players immediately understand what is ahead without visual noise.
   - Paced spawn intervals: starting at 180 frames (~3.0 seconds) and ramping down gradually to 60 frames (~1.0 second), with generous 55–120px spawn radii biased in player flight direction.
2. **Collectibles Overhaul (Money)**:
   - **Diamond Gem** (Frame 12): Crisp 16x16 faceted gemstone (+10 points base $\times$ combo multiplier).
   - **Dollar Bills** (Frame 11): 16x16 stack of bills with `$` emblem (+25 points base $\times$ combo multiplier).
   - **Discarded Money Bag**: Replaced completely with the new Diamond and Bills icons.
3. **8 Distinct Food Hazards**:
   - Repurposed existing food icons from [items_16.png](file:///home/dorito/Developer/arduboy/supermassive-whitehole/assets/items_16.png) into 8 balanced obstacles with varying slow intensities and durations:
     - **Apple**: Quick snack ($-10\%$ speed, 0.4s)
     - **Pizza**: Light slice ($-15\%$ speed, 0.5s)
     - **Taco**: Spicy kick ($-25\%$ speed, 0.75s)
     - **Burger**: Medium meal ($-30\%$ speed, 1.0s)
     - **French Fries**: Salty delay ($-35\%$ speed, 1.25s)
     - **Cake**: Sugar crash ($-45\%$ speed, 1.75s)
     - **Donut**: Heavy sweet ($-50\%$ speed, 1.5s)
     - **Ice Cream**: Brain freeze ($-60\%$ speed, 2.0s)
4. **Future Milestone Context (M3)**:
   - Prepared entity systems for M3 where the Whitehole will absorb money and food to increase its mass and charge.

---

## Visual Verification

### In-Game Diamond & Spacious Runway
![Faceted Diamond and Spacious Runway](file:///home/dorito/.gemini/antigravity-ide/brain/9e04b54b-8891-4396-b00f-9846df8b76cc/final_diamond_in_game_1789601612572.png)

*The player centered on the perspective grid with the faceted Diamond gem clearly framed ahead in the central runway, well clear of the Whitehole on the left and the HUD on the right.*

### Updated 16x16 Spritesheet
![16x16 Items Spritesheet](file:///home/dorito/.gemini/antigravity-ide/brain/9e04b54b-8891-4396-b00f-9846df8b76cc/items_16_preview.png)

*Row 1 (top): Chicken, Pizza, Burger, Taco/Hotdog, Apple/Berry, Cherries, Ice Cream, Watermelon, Waffle.*
*Row 2 (bottom): Coffee Mug, Candy, **Dollar Bills ($)**, **Faceted Diamond**, Soda/Drink, Donut, Grapes, Cake, Pretzel.*

---

## Technical Changes by Component

### 1. Configuration & Constants ([config.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/config.h))
- Added `FOOD_APPLE_SIZE` through `FOOD_ICECREAM_SIZE` (8px hitboxes).
- Added individual slow durations and intensities for all 8 food hazards (`FOOD_APPLE_SLOW_DURATION` / `INTENSITY` through `FOOD_ICECREAM_SLOW_DURATION` / `INTENSITY`).
- Tuned spawn intervals: `SPAWN_INTERVAL_START = 180`, `SPAWN_MIN_INTERVAL = 60`, `SPAWN_RAMP_INTERVAL = 240`.
- Tuned distances: `SPAWN_MIN_DISTANCE = 55`, `SPAWN_RADIUS = 120`, and `MIN_ITEM_SEPARATION = 45`.
- Added `SCORE_PER_DIAMOND = 10` and `SCORE_PER_BILLS = 25`.

### 2. Entity Model & Manager ([entity.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/entity.h), [entity.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/entity.cpp))
- Expanded `EntityType` enum with `ENTITY_COLLECTIBLE_DIAMOND`, `ENTITY_COLLECTIBLE_BILLS`, `ENTITY_POWERUP_COFFEE`, and `ENTITY_FOOD_APPLE` through `ENTITY_FOOD_ICECREAM`.
- Updated `isFood()`, `isCollectible()`, `isPowerup()` to classify all new types accurately.
- Updated `EntityManager::spawn()` to assign appropriate hitboxes.

### 3. Player Slow Debuff Handler ([player.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/player.cpp))
- Updated `Player::applyFoodSlow()` with a switch covering all 8 food hazard types, applying their specific durations and speed reduction percentages.

### 4. Game Orchestrator & Spawning ([game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp))
- Updated `Game::reset()` to spawn only 2 spacious starting items (`ENTITY_COLLECTIBLE_DIAMOND` and `ENTITY_FOOD_PIZZA`), positioned $>60$ units apart.
- Implemented `MIN_ITEM_SEPARATION` enforcement in `Game::updatePlaying()`: up to 4 candidate positions are tested to ensure no new entity spawns within 45 units of any existing entity.
- Updated entity type distribution:
  - $30\%$ Diamond Gem (+10 pts $\times$ combo)
  - $10\%$ Dollar Bills (+25 pts $\times$ combo)
  - $15\%$ Coffee Power-Up (+speed boost & cleanse)
  - $45\%$ Food Hazards (split across the 8 hazards)
- Updated scoring in collisions to award `SCORE_PER_DIAMOND` or `SCORE_PER_BILLS` multiplied by `comboMultiplier`.
- Updated `Game::renderEntities()` to map each entity type to its respective PROGMEM frame.

### 5. Sprites & Conversion ([convert_sprites.py](file:///home/dorito/Developer/arduboy/supermassive-whitehole/scripts/convert_sprites.py), [sprites.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/assets/sprites.h))
- Converted Dollar Bills (`$`) to Frame 11 and Diamond to Frame 12.
- Cleaned 1-bit RGB pixel data and regenerated `assets/sprites.h` with dedicated frame index constants.

---

## Verification & Resource Budget

### Hardware Limits (Arduboy / ATmega32u4)
| Resource | Used | Maximum | Percentage | Headroom |
|---|---|---|---|---|
| **Program Storage (Flash)** | 17,598 bytes | 28,672 bytes | 61.3% | 11,074 bytes |
| **Dynamic Memory (SRAM)** | 1,483 bytes | 2,560 bytes | 57.9% | 1,077 bytes |

### Quality & Performance Verification
- [x] Clean compilation for AVR target with zero warnings or errors.
- [x] Tested in web emulator: items are spacious and easily distinguishable at high speed.
- [x] Faceted Diamond and Dollar Bills render crisply with distinct 1-bit pixel art.
- [x] All 8 food hazards apply their unique slow durations and intensities.
- [x] Zero dynamic allocations (`new`/`malloc`), preserving 1,077 bytes of SRAM headroom.
