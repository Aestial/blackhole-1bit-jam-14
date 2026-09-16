# Delta-Time Integration & Movement/Animation Tuning Implementation Plan

Make all time-dependent computations platform- and CPU-independent by integrating a normalized fixed-point delta time (`dt`), slowing down sprite animations to a natural pace, and tuning player/enemy speeds for balanced gameplay.

## User Review Required

> [!IMPORTANT]
> - **Delta-Time Architecture**: We will compute `dt` using hardware wall-clock time (`millis()`), normalized so that `1.0` in Q8.8 fixed-point (`FP_ONE = 256`) represents 1 standard frame at 60 FPS (16.67 ms). This guarantees that game physics, timers, and animations run at the exact same physical speed regardless of CPU clock rate, display refresh rate, or browser emulator throttling.
> - **Animation Speed**: Whitehole rotation speed will be reduced by 3× (from 1 frame every 4 ticks to 1 frame every 12 ticks), creating a majestic, steady cosmic swirl (~0.8s per full rotation) rather than a high-speed blur.
> - **Movement Speed Tuning**:
>   - Player max speed reduced from 2.0 px/frame (120 px/s) to 1.0 px/frame (60 px/s) so maneuvering on the 128×64 screen feels controlled, heavy, and deliberate.
>   - Player acceleration tuned from 0.15 to 0.08, giving smooth inertia ramp-up (~0.2s to reach max speed).
>   - Enemy (whitehole/blackhole) chase base speed tuned from 0.30 to 0.20 px/frame (12 px/s), giving the player room to collect food and navigate before the whitehole gradually gains speed.

---

## Proposed Changes

### Configuration & Physics Constants

#### [MODIFY] [src/game/config.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/config.h)
- Add delta-time helper macros:
  - `DT_TO_MS(dt)` and normalized fixed-point delta time definitions.
- Retune player physics constants:
  - `PLAYER_ACCEL`: `0.15` → `0.08`
  - `PLAYER_FRICTION`: `0.02` → `0.015`
  - `PLAYER_BRAKE_FRICTION`: `0.08` → `0.05`
  - `PLAYER_MAX_SPEED`: `2.0` → `1.0`
- Retune enemy physics constants:
  - `BH_BASE_SPEED`: `0.3` → `0.20`
  - `BH_SPEED_INCREMENT`: Ramped over seconds rather than every tick so difficulty scales smoothly over a 60–90 second survival run.
- Animation speed constants:
  - `WHITEHOLE_ANIM_DIVISOR`: `12` (was hardcoded `4`)
  - `PLAYER_BLINK_DIVISOR`: `8` (was hardcoded `4`)

---

### Main Loop & Delta Time Calculation

#### [MODIFY] [supermassive-whitehole.ino](file:///home/dorito/Developer/arduboy/supermassive-whitehole/supermassive-whitehole.ino)
- Track `lastMillis` across frames.
- Calculate elapsed wall-clock milliseconds clamped to `[1, 100]` ms (guards against pauses/background tabs).
- Calculate normalized Q8.8 delta time:
  `fp_t dt = (fp_t)(((uint32_t)elapsed * 384) / 25);`
- Pass `dt` to `game.update(input, dt)`.

---

### Game Logic & Physics Systems

#### [MODIFY] [src/game/physics.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/physics.h) & [src/game/physics.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/physics.cpp)
- Update `moveToward()` to accept `fp_t dt` and scale displacement by `dt`:
  `void moveToward(fp32_t& x, fp32_t& y, fp32_t targetX, fp32_t targetY, fp_t speed, fp_t dt);`

#### [MODIFY] [src/game/player.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/player.h) & [src/game/player.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/player.cpp)
- Update `Player::update()` signature to accept `fp_t dt`.
- Scale thrust acceleration by `dt`: `vx += FP_MUL(desiredDx * PLAYER_ACCEL, dt)`.
- Scale friction deceleration by `dt`: `vx -= FP_MUL(friction, dt)`.
- Scale position integration by `dt`: `x += FP32_MUL(vx, dt)`.
- Update slow effect countdown with delta time.

#### [MODIFY] [src/game/world.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/world.h) & [src/game/world.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/world.cpp)
- Update `World::update()` to accept `fp_t dt`.
- Pass `dt` to `moveToward()`.
- Update `bhSpin` with `dt` accumulation.
- Scale `spawnTimer`, `scoreTimer`, and `gameTime` with delta time.

#### [MODIFY] [src/game/game.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.h) & [src/game/game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp)
- Update `Game::update()` to `update(HalInput& input, fp_t dt)`.
- In `updateTitle()`: advance `world.bhSpin` by `dt`.
- In `renderTitle()` and `renderBlackhole()`: use `WHITEHOLE_ANIM_DIVISOR` (12) for smooth, slower swirling.
- In `renderPlayer()`: use `PLAYER_BLINK_DIVISOR` (8) for gentler blink cadence.

---

## Verification Plan

### Automated Build
- Compile with `./build.sh --build-only` to verify zero build warnings/errors and check Flash/RAM limits.

### Manual Verification
- Test in ProjectABE web emulator:
  1. Title screen whitehole animation rotates at a steady, slower, mesmerizing pace.
  2. Pressing A starts the game; player movement feels smooth, deliberate, and manageable rather than rocket-fast.
  3. Enemy moves steadily without instantly overwhelming the player.
  4. Test with browser throttled or on different display refresh rates (e.g. 60 Hz vs 144 Hz) to ensure gameplay speed remains uniform.
- Capture updated screenshots to [dist/assets/](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/assets/).
