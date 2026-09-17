# Milestone 2 (M2) Update — Whitehole Risk-Reward Money Distribution Curve

## Summary of Completed Work
To establish a high-stakes arcade dynamic and prepare for Milestone 3 (where the Whitehole attracts and absorbs objects), we implemented a **Whitehole Risk-Reward Money Distribution Curve**:
1. **Whitehole Orbit Proximity ($80\%$ of Money Spawns)**:
   - Money collectibles (**Diamond Gems** and **Dollar Bills**) now spawn primarily in an orbital danger ring surrounding the Whitehole, rather than scattered randomly across empty space.
   - Food hazards (Apple, Pizza, Taco, Burger, Fries, Cake, Donut, Ice Cream) and Coffee power-ups continue to spawn along the player's forward flight path.
2. **Attraction Force Safety Clearance**:
   - The inner radius of the money spawn ring is anchored to the Whitehole's attraction radius (`bhCharge`, initially 30 px) with a safety buffer (`MONEY_BH_SAFE_BUFFER = 20` px):
     $$R_{safe\_min} = \text{FP\_TO\_INT}(bhCharge) + \text{MONEY\_BH\_SAFE\_BUFFER} = 50\text{ px}$$
   - This ensures that when the money spawns (and when the player swoops in to collect it), neither the item nor the player gets caught in the gravitational pull or hits the core hitbox.
   - **Dynamic M3 Scaling**: As the Whitehole grows in mass and charge over time in Milestone 3, this safe perimeter automatically scales outward in tandem.
3. **Decaying Distribution Curve ($\Delta r = \min(u_1, u_2)$)**:
   - Within the ring ($R \in [R_{safe\_min}, R_{safe\_min} + 40]$), radial distance is sampled using the minimum of two uniform random variables.
   - This yields a linearly decaying probability density function:
     $$P(\Delta r \le x) = 1 - \left(1 - \frac{x}{W}\right)^2$$
   - **$75\%$ of all money spawns concentrate in the inner half of the ring** ($50\text{--}70$ px from Whitehole), creating a visible "treasure halo" close to the danger zone, with only $25\%$ in the outer half ($70\text{--}90$ px).
4. **Isotropic Placement via 16-Point Unit Circle**:
   - Exact Euclidean radial positioning is computed using a 16-point unit circle lookup table (`UNIT_CIRCLE_X`, `UNIT_CIRCLE_Y`), preventing distortion while avoiding expensive runtime trigonometric functions on the AVR chip.
5. **Spacing Integrity**:
   - Continues to enforce `MIN_ITEM_SEPARATION = 45` world units against all active entities.

---

## Visual Verification

### Diamond Floating in Whitehole Risk-Reward Orbit
![Diamond in Whitehole Orbit](file:///home/dorito/.gemini/antigravity-ide/brain/9e04b54b-8891-4396-b00f-9846df8b76cc/diamond_orbit_verified.png)

*The game immediately illustrates the risk-reward dynamic: the swirling Whitehole looms on the left, the faceted Diamond gem floats safely in its orbital perimeter (screen X $\approx 43$), and the player is positioned in the center lane (screen X $= 64$).*

---

## Technical Changes by Component

### 1. Configuration Constants ([config.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/config.h))
- Added `MONEY_BH_SPAWN_CHANCE = 80`: $80\%$ of money items target the Whitehole danger ring.
- Added `MONEY_BH_SAFE_BUFFER = 20`: Safe offset beyond `bhCharge` (30 px $\to$ 50 px minimum).
- Added `MONEY_BH_RING_SPAN = 40`: Width of the orbit ring for the decaying distribution curve.
- Added `UNIT_CIRCLE_X[16]` and `UNIT_CIRCLE_Y[16]`: 16-direction normalized unit vectors for circular placement without runtime square roots or trig calculations.

### 2. Game Orchestration & Spawning ([game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp))
- **`Game::reset()`**:
  - Pre-spawns the starting Diamond at `world.bhX + 50, world.bhY - 15` (52 px from Whitehole, safely clear of its 30 px charge), visibly framing the treasure in the danger orbit.
  - Spawns the starting Pizza hazard $>70$ px ahead in the player's runway.
- **`Game::updatePlaying()`**:
  - Detects if the rolled entity is a money item (`ENTITY_COLLECTIBLE_DIAMOND` or `ENTITY_COLLECTIBLE_BILLS`).
  - Implements the dual-random decaying distribution curve: $\Delta r = \min(u_1, u_2)$ added to `minSafeDist`.
  - Computes world coordinates using `UNIT_CIRCLE` offsets.
  - Guards against spawning within 35 px of the player or beyond active camera boundaries.
  - Falls back to forward player path on attempt 3 if the Whitehole perimeter is congested.

---

## Verification & Resource Budget

### Hardware Limits (Arduboy / ATmega32u4)
| Resource | Used | Maximum | Percentage | Headroom |
|---|---|---|---|---|
| **Program Storage (Flash)** | 17,792 bytes | 28,672 bytes | 62.0% | 10,880 bytes |
| **Dynamic Memory (SRAM)** | 1,515 bytes | 2,560 bytes | 59.1% | 1,045 bytes |

### Quality & Performance Checks
- [x] Clean compilation for AVR target (`arduboy-homemade:avr:arduboy`) with zero warnings or errors.
- [x] Tested in web emulator: Diamond appears in the orbit between Whitehole and Player at start.
- [x] Money spawns cluster near the Whitehole's perimeter without overlapping its attraction field.
- [x] $100\%$ fixed-point and integer math; zero dynamic memory allocations.
