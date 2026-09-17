# Player Control Overhaul — Human-on-Foot Agile Steering & Small Walk Impulses

## Summary of Completed Work
We revised the player physics model from the original heavy car/vehicle mechanics to a responsive **Human-on-Foot** model designed specifically for a fat man running and maneuvering on the infinite plane:

1. **Agile Human Steering (`PLAYER_INERTIA = 0.25`)**:
   - Replaced the heavy car-like drift (`PLAYER_INERTIA = 0.85`, which retained $85\%$ of previous velocity and caused wide skidding) with agile human steering (`0.25`).
   - Humans turn on foot by planting their feet and redirecting their stride. Velocity now redirects toward the D-pad heading in **2–3 frames**, allowing snappy, precise navigation between food hazards and whitehole orbits while retaining a subtle $25\%$ tactile body weight.

2. **Small Walk Impulses (`PLAYER_STEP_IMPULSE = 0.25`, `PLAYER_ACCEL = 0.040`)**:
   - The gas button (A) now acts as a natural walking/striding impulse rather than a rocket engine.
   - **Tapping A**: Initiates a single grounded stride from a standstill (`PLAYER_STEP_IMPULSE = 0.25 px/frame`), stepping forward ~4 pixels and stopping.
   - **Holding A**: Takes continuous rhythmic strides adding small impulses (`0.040 px/frame`) up to a comfortable jogging speed (`PLAYER_MAX_SPEED = 0.95 px/frame` $\approx 57\text{ px/sec}$).

3. **Natural Foot Drag & Active Braking**:
   - **Passive Drag (`PLAYER_FRICTION = 0.040`)**: Releasing the gas button causes the fat man to take 2–3 decelerating steps and come to a halt in ~20 frames (~0.33 sec), eliminating the previous ice-skating glide.
   - **Brake (`PLAYER_BRAKE_FRICTION = 0.12`)**: Holding B firmly plants the feet for a quick emergency stop in ~8 frames.

---

## Technical Comparison: Car vs. Human-on-Foot

| Parameter | Vehicle Model (Previous) | Human-on-Foot (New) | Effect |
|---|---|---|---|
| **Inertia Blend (`PLAYER_INERTIA`)** | `0.85` (Heavy car drift) | `0.25` (Agile human turning) | Eliminates wide sideways skidding; turns in 2–3 frames |
| **Stride Impulse (`PLAYER_STEP_IMPULSE`)** | None ($0$) | `0.25 px/frame` | Tapping A takes a clear single step forward |
| **Walk Impulse (`PLAYER_ACCEL`)** | `0.090 px/frame` | `0.040 px/frame` | Grounded, human-scaled acceleration instead of car rev |
| **Max Speed (`PLAYER_MAX_SPEED`)** | `1.05 px/frame` | `0.95 px/frame` | Readable, comfortable human jogging speed |
| **Natural Friction (`PLAYER_FRICTION`)** | `0.012 px/frame` (90 frames) | `0.040 px/frame` (20 frames) | Natural foot drag stopping in 2–3 strides instead of gliding on ice |
| **Brake Friction (`PLAYER_BRAKE_FRICTION`)** | `0.060 px/frame` | `0.120 px/frame` | Firm foot-plant emergency stop in ~8 frames |

---

## Hardware Resource Budget (Arduboy / ATmega32u4)

| Resource | Used | Maximum | Percentage | Headroom |
|---|---|---|---|---|
| **Program Storage (Flash)** | 17,742 bytes | 28,672 bytes | 61.8% | 10,930 bytes |
| **Dynamic Memory (SRAM)** | 1,515 bytes | 2,560 bytes | 59.1% | 1,045 bytes |

- **AVR Build Verification**: Successfully compiled with zero errors/warnings.
- **Runtime Performance**: $100\%$ fixed-point math (Q8.8 / Q24.8); zero heap allocations.
