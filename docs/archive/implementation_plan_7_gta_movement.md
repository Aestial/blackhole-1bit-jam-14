# GTA-Style Player Movement: Walk / Run / Sprint + Stamina System

> **Status:** APPROVED — Ready for implementation by agents.
>
> Overhaul the player movement input model from the current "D-pad + A=gas" scheme to a **GTA-style 3-tier locomotion** system with **variable sprint speed proportional to tap frequency**, a **stamina resource**, **stamina HUD bar**, **game-over cooldown screen**, and **entity interactions** (coffee recovers stamina, fast food drains it).

---

## Design Decisions (Resolved)

These were open questions that have been answered by the project owner:

1. **Walk speed (D-pad only):** Set to ~40% of max speed — just barely enough to outrun the blackhole at the start of the game. The goal is to **encourage players to run and eventually sprint** as the blackhole speeds up.

2. **Sprint speed is VARIABLE, not binary.** Sprint speed should scale proportionally with tapping frequency. Faster tapping = faster sprinting, up to the max sprint speed. This requires polling tap intervals and mapping them to a speed multiplier. See the detailed algorithm below.

3. **Stamina regen:** GTA:SA style — regen at all times. **Fastest when idle** (standing still), slower when walking. No regen while running or sprinting.

4. **Game-over → Title transition:** Add a **brief loading/cooldown screen** (1–2 seconds, no input accepted) between the game-over state and the title/menu screen. This prevents accidental menu selection from residual sprint-tapping when the player dies.

5. **Brake (B button) removed from gameplay:** The previous brake mechanic (holding B for instant foot-plant deceleration `PLAYER_BRAKE_FRICTION`) is completely removed. In GTA:SA on-foot movement, there is no brake button — releasing directional input naturally decelerates the character via foot drag (`PLAYER_FRICTION`). B button is no longer polled during active gameplay, but remains used for menu navigation (e.g., "Retry" on the Game Over screen).

---

## Architecture Overview

### Movement Tier System

| Tier | Input | Speed | Stamina Cost | Animation Cadence |
|------|-------|-------|-------------|-------------------|
| **Walk** | D-pad only (no A) | `PLAYER_WALK_SPEED` (~40% max) | None (regen active) | Slow walk cycle |
| **Run** | D-pad + A held | `PLAYER_RUN_SPEED` (~75% max) | Moderate drain | Normal walk cycle |
| **Sprint** | D-pad + A tapped rapidly | Variable: 75%–100% of max, proportional to tap speed | Heavy drain | Fast walk cycle |

### Sprint Speed ↔ Tap Frequency Mapping

Sprint speed is **not a fixed value** — it interpolates between `PLAYER_RUN_SPEED` and `PLAYER_SPRINT_SPEED` based on how fast the player taps A:

```
tapInterval = frames between last two A presses

If tapInterval >= SPRINT_TAP_WINDOW (15 frames):
    → Not sprinting (just running)

If tapInterval < SPRINT_TAP_WINDOW:
    → Sprint active
    → speedFactor = 1.0 - (tapInterval / SPRINT_TAP_WINDOW)
       Maps: tapInterval=1 → speedFactor≈1.0 (max sprint)
              tapInterval=14 → speedFactor≈0.07 (barely above run)
    → effectiveSpeed = PLAYER_RUN_SPEED + speedFactor * (PLAYER_SPRINT_SPEED - PLAYER_RUN_SPEED)
```

In Q8.8 fixed-point:
```cpp
fp_t speedRange = PLAYER_SPRINT_SPEED - PLAYER_RUN_SPEED;
fp_t factor = INT_TO_FP(SPRINT_TAP_WINDOW - tapInterval) / SPRINT_TAP_WINDOW;
fp_t sprintSpeed = PLAYER_RUN_SPEED + FP_MUL(speedRange, factor);
```

When stamina is depleted:
- **Sprint** → forced downgrade to **Run**
- **Run** → forced downgrade to **Walk**
- Visual feedback: stamina bar blinks

---

## Proposed Changes

### Component 1: Config Constants

#### [MODIFY] [config.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/config.h)

1. **Remove `PLAYER_BRAKE_FRICTION`**: The brake mechanic is obsolete with GTA:SA on-foot movement. Remove line ~164-165:
```diff
- static const fp_t PLAYER_BRAKE_FRICTION =
-     FLOAT_TO_FP(0.12);                                  // Foot-plant brake drag
```
*(Natural foot friction `PLAYER_FRICTION` will handle all coasting deceleration when directional input ceases).*

2. Add new constant sections **after** the existing `PLAYER PHYSICS CONSTANTS` block (after line ~169). Keep the existing `PLAYER_MAX_SPEED` constant unchanged (sprint max equals it).

**Add these new sections:**

```cpp
// =============================================================================
// PLAYER SPEED TIERS (GTA-STYLE LOCOMOTION)
// =============================================================================
// Walk:   D-pad only, no stamina cost, slow but safe — barely outruns the
//         initial blackhole (BH_BASE_SPEED = 0.20). Encourages players to
//         run/sprint as difficulty ramps.
// Run:    D-pad + A held, moderate stamina drain, comfortable pace.
// Sprint: D-pad + A tapped rapidly, heavy stamina drain. Speed scales
//         proportionally with tap frequency (faster taps = faster sprint).
static const fp_t PLAYER_WALK_SPEED   = FLOAT_TO_FP(0.38);  // ~40% of max
static const fp_t PLAYER_RUN_SPEED    = FLOAT_TO_FP(0.72);  // ~75% of max
static const fp_t PLAYER_SPRINT_SPEED = FLOAT_TO_FP(0.95);  // 100% (== PLAYER_MAX_SPEED)

// Per-tier acceleration values
static const fp_t PLAYER_WALK_ACCEL   = FLOAT_TO_FP(0.025); // Slow, heavy stroll
static const fp_t PLAYER_RUN_ACCEL    = FLOAT_TO_FP(0.040); // Same as original PLAYER_ACCEL
static const fp_t PLAYER_SPRINT_ACCEL = FLOAT_TO_FP(0.055); // Burst acceleration

// =============================================================================
// SPRINT TAP DETECTION CONSTANTS
// =============================================================================
// Sprint is activated by tapping A rapidly (release → re-press within window).
// Sprint SPEED scales with tap FREQUENCY — faster taps = faster sprint.
//
// State machine: track frames between consecutive A presses (tapInterval).
//   tapInterval <  SPRINT_TAP_WINDOW → sprinting (speed proportional to 1/tapInterval)
//   tapInterval >= SPRINT_TAP_WINDOW → not sprinting (just running if A held)
//
// A minimum of SPRINT_TAP_COUNT_NEEDED consecutive fast taps is required
// before entering sprint mode (prevents accidental single fast press).
static const uint8_t SPRINT_TAP_WINDOW      = 15;  // Max frames between A presses to count as sprint tap (~250ms at 60 FPS)
static const uint8_t SPRINT_TAP_COUNT_NEEDED = 2;   // Fast taps needed before sprint activates
static const uint8_t SPRINT_SUSTAIN_WINDOW   = 20;  // If A is HELD (not tapped) for this many frames, downgrade to run

// =============================================================================
// STAMINA SYSTEM CONSTANTS
// =============================================================================
// Stamina is a uint8_t (0–255) representing the player's endurance pool.
// Depletes while running/sprinting, regenerates passively over time.
//
// GTA:SA-style regen behavior:
//   - Idle:    fastest regen (STAMINA_REGEN_IDLE per tick)
//   - Walk:    slower regen  (STAMINA_REGEN_WALK per tick)
//   - Run:     NO regen, moderate drain
//   - Sprint:  NO regen, heavy drain
//   - Coffee:  instant stamina boost
//   - Food:    instant stamina loss (in addition to speed slow debuff)
static const uint8_t STAMINA_MAX             = 255;
static const uint8_t STAMINA_START           = 255;  // Start fully rested

// Drain rates: stamina points lost per drain tick
static const uint8_t STAMINA_RUN_DRAIN       = 1;
static const uint8_t STAMINA_SPRINT_DRAIN    = 3;

// Drain tick intervals: frames between each drain tick (higher = slower drain)
// Run:    drain 1 point every 10 frames → ~42 sec to deplete from full
// Sprint: drain 3 points every 4 frames → ~5.7 sec to deplete from full
static const uint8_t STAMINA_RUN_DRAIN_INTERVAL    = 10;
static const uint8_t STAMINA_SPRINT_DRAIN_INTERVAL  = 4;

// Regen tick values and interval
// Idle:  regen 2 every 15 frames → ~32 sec to fully recover
// Walk:  regen 1 every 15 frames → ~64 sec to fully recover
static const uint8_t STAMINA_REGEN_IDLE      = 2;
static const uint8_t STAMINA_REGEN_WALK      = 1;
static const uint8_t STAMINA_REGEN_INTERVAL  = 15;

// Minimum stamina required to INITIATE running or sprinting
// (once started, you drain until 0 before forced downgrade)
static const uint8_t STAMINA_MIN_TO_RUN      = 10;
static const uint8_t STAMINA_MIN_TO_SPRINT   = 25;

// Coffee powerup stamina recovery (added on top of existing speed boost)
static const uint8_t STAMINA_COFFEE_RECOVERY = 80;

// Food hazard stamina drain (added on top of existing speed slow debuff)
static const uint8_t STAMINA_FOOD_APPLE_DRAIN    = 5;
static const uint8_t STAMINA_FOOD_PIZZA_DRAIN    = 15;
static const uint8_t STAMINA_FOOD_TACO_DRAIN     = 20;
static const uint8_t STAMINA_FOOD_BURGER_DRAIN   = 30;
static const uint8_t STAMINA_FOOD_FRIES_DRAIN    = 25;
static const uint8_t STAMINA_FOOD_CAKE_DRAIN     = 40;
static const uint8_t STAMINA_FOOD_DONUT_DRAIN    = 35;
static const uint8_t STAMINA_FOOD_ICECREAM_DRAIN = 50;

// =============================================================================
// STAMINA BAR HUD CONSTANTS
// =============================================================================
// Vertical bar on the right side of the screen showing current stamina.
// Fills from bottom (empty) to top (full). Blinks when critically low.
static const uint8_t STAMINA_BAR_X       = 124;  // X position (right edge)
static const uint8_t STAMINA_BAR_Y       = 4;    // Y position (top)
static const uint8_t STAMINA_BAR_WIDTH   = 3;    // Width in pixels
static const uint8_t STAMINA_BAR_HEIGHT  = 40;   // Height in pixels
static const uint8_t STAMINA_CRITICAL_THRESHOLD = 38;  // ~15% — bar blinks below this

// =============================================================================
// GAME-OVER COOLDOWN CONSTANTS
// =============================================================================
// Brief no-input cooldown screen after game over, before showing title/menu.
// Prevents accidental menu selection from residual sprint-tapping when the
// player dies while frantically tapping A to escape the whitehole.
static const uint8_t GAMEOVER_COOLDOWN_FRAMES = 90;  // ~1.5 sec at 60 FPS
```

---

### Component 2: Player State & Logic

#### [MODIFY] [player.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/player.h)

**Add `MoveTier` enum** before the `Player` struct definition (after line ~76):

```cpp
// Movement tier enum (GTA-style locomotion)
enum MoveTier : uint8_t {
    MOVE_IDLE   = 0,   // Standing still
    MOVE_WALK   = 1,   // D-pad only — slow, no stamina cost
    MOVE_RUN    = 2,   // D-pad + A held — moderate speed, stamina drains
    MOVE_SPRINT = 3    // D-pad + A tapped rapidly — variable speed, heavy stamina drain
};
```

**Add new fields to `Player` struct** (after `boostTimerAccum`, ~line 103):

```cpp
// ---- Stamina system (GTA:SA-style) ----
uint8_t stamina;            // Current stamina (0–STAMINA_MAX)
uint8_t staminaDrainAccum;  // Frame accumulator for drain tick timing
uint8_t staminaRegenAccum;  // Frame accumulator for regen tick timing

// ---- Sprint tap detection state machine ----
MoveTier moveTier;           // Current movement tier (IDLE/WALK/RUN/SPRINT)
uint8_t  sprintTapTimer;     // Frames since last A press (for measuring tap interval)
uint8_t  sprintTapInterval;  // Frames between the last two A presses (for speed scaling)
uint8_t  sprintTapCount;     // Consecutive fast taps counted
bool     aPrevPressed;       // A button state previous frame (for edge detection)
uint8_t  aHoldTimer;         // Frames A has been held continuously (detect hold vs tap)
```

**Modify `update()` signature** — replace `brake` with `justPressedA`:

```cpp
void update(bool up, bool down, bool left, bool right,
            bool accel, bool justPressedA, fp_t dt = FP_DT_ONE);
```

> [!NOTE]
> `bool brake` is removed from the signature. In GTA:SA on-foot locomotion, there is no active braking button.

**Add new methods:**

```cpp
// drainStamina(amount) — Subtract stamina, clamped to 0
void drainStamina(uint8_t amount);

// recoverStamina(amount) — Add stamina, clamped to STAMINA_MAX
void recoverStamina(uint8_t amount);

// getStaminaPercent() — Returns 0–100 for HUD display
uint8_t getStaminaPercent() const;

// getCurrentMoveTier() — Returns current tier for HUD/animation/visual effects
MoveTier getCurrentMoveTier() const;
```

---

#### [MODIFY] [player.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/player.cpp)

This is the largest change. The `update()` function is substantially rewritten.

##### `Player::init()` — Add stamina/sprint initialization

Append after existing init code:

```cpp
stamina = STAMINA_START;
staminaDrainAccum = 0;
staminaRegenAccum = 0;
moveTier = MOVE_IDLE;
sprintTapTimer = 255;     // Expired — no recent tap
sprintTapInterval = 255;  // No measured interval yet
sprintTapCount = 0;
aPrevPressed = false;
aHoldTimer = 0;
```

##### `Player::update()` — Full rewrite

The new algorithm (replace the existing body, keeping the same structure):

```
STEP 1: Set desiredDx/desiredDy from D-pad (UNCHANGED from current code)
STEP 2: Update 8-directional facing (UNCHANGED from current code)

STEP 3: Sprint tap detection state machine
   ┌─────────────────────────────────────────────────────────────────┐
   │ This runs every frame to track A-button tap patterns.          │
   │                                                                 │
   │ a) Detect A PRESS edge (justPressedA == true):                 │
   │    - Record sprintTapInterval = sprintTapTimer (frames since   │
   │      last press)                                                │
   │    - If sprintTapInterval < SPRINT_TAP_WINDOW:                 │
   │        sprintTapCount++  (fast tap detected)                   │
   │    - Else:                                                      │
   │        sprintTapCount = 1  (too slow, reset)                   │
   │    - Reset sprintTapTimer = 0                                   │
   │    - Reset aHoldTimer = 0                                       │
   │                                                                 │
   │ b) While A is held (accel == true):                            │
   │    - Increment aHoldTimer each frame                           │
   │    - If aHoldTimer > SPRINT_SUSTAIN_WINDOW:                    │
   │        Player is HOLDING, not tapping → downgrade to RUN       │
   │                                                                 │
   │ c) Increment sprintTapTimer (cap at 255 to prevent overflow)   │
   │                                                                 │
   │ d) Save aPrevPressed = accel  (for next frame edge detection)  │
   └─────────────────────────────────────────────────────────────────┘

STEP 4: Determine movement tier
   ┌─────────────────────────────────────────────────────────────────┐
   │ bool hasDir = (desiredDx != 0 || desiredDy != 0);              │
   │                                                                 │
   │ if (!hasDir):                                                   │
   │     moveTier = MOVE_IDLE                                        │
   │                                                                 │
   │ else if (!accel):  // D-pad only, no A button                  │
   │     moveTier = MOVE_WALK                                        │
   │                                                                 │
   │ else if (sprintTapCount >= SPRINT_TAP_COUNT_NEEDED             │
   │          && aHoldTimer <= SPRINT_SUSTAIN_WINDOW                │
   │          && stamina >= STAMINA_MIN_TO_SPRINT):                 │
   │     moveTier = MOVE_SPRINT                                      │
   │                                                                 │
   │ else if (stamina >= STAMINA_MIN_TO_RUN):                       │
   │     moveTier = MOVE_RUN                                         │
   │                                                                 │
   │ else:                                                           │
   │     moveTier = MOVE_WALK  // Stamina too low, forced walk      │
   └─────────────────────────────────────────────────────────────────┘

STEP 5: Compute effective speed and acceleration from tier
   ┌─────────────────────────────────────────────────────────────────┐
   │ fp_t maxSpd, accelVal;                                          │
   │                                                                 │
   │ switch (moveTier):                                              │
   │   MOVE_IDLE:                                                    │
   │     maxSpd = 0; accelVal = 0;                                   │
   │                                                                 │
   │   MOVE_WALK:                                                    │
   │     maxSpd = PLAYER_WALK_SPEED;                                 │
   │     accelVal = PLAYER_WALK_ACCEL;                               │
   │                                                                 │
   │   MOVE_RUN:                                                     │
   │     maxSpd = PLAYER_RUN_SPEED;                                  │
   │     accelVal = PLAYER_RUN_ACCEL;                                │
   │                                                                 │
   │   MOVE_SPRINT:                                                  │
   │     // VARIABLE SPEED based on tap frequency!                   │
   │     // Faster taps (lower sprintTapInterval) → faster speed    │
   │     fp_t speedRange = PLAYER_SPRINT_SPEED - PLAYER_RUN_SPEED;  │
   │     uint8_t clampedInterval = min(sprintTapInterval,            │
   │                                   SPRINT_TAP_WINDOW);           │
   │     // factor: 0 (slowest sprint) to FP_ONE (fastest sprint)   │
   │     fp_t factor = INT_TO_FP(SPRINT_TAP_WINDOW - clampedInterval)│
   │                   / SPRINT_TAP_WINDOW;                          │
   │     maxSpd = PLAYER_RUN_SPEED + FP_MUL(speedRange, factor);    │
   │     accelVal = PLAYER_SPRINT_ACCEL;                             │
   └─────────────────────────────────────────────────────────────────┘

STEP 6: Apply boost/slow modifiers (KEEP existing logic)
   - If boostTimer > 0: multiply maxSpd and accelVal by boost factors
   - If slowTimer > 0: reduce maxSpd by slowIntensity percentage
   - Decrement boost/slow timers with delta-time accumulators

STEP 7: Stamina drain and regeneration
   ┌─────────────────────────────────────────────────────────────────┐
   │ if (moveTier == MOVE_SPRINT):                                   │
   │     staminaDrainAccum++                                         │
   │     if (staminaDrainAccum >= STAMINA_SPRINT_DRAIN_INTERVAL):   │
   │         drainStamina(STAMINA_SPRINT_DRAIN)                      │
   │         staminaDrainAccum = 0                                   │
   │     staminaRegenAccum = 0  // No regen while sprinting         │
   │                                                                 │
   │ else if (moveTier == MOVE_RUN):                                │
   │     staminaDrainAccum++                                         │
   │     if (staminaDrainAccum >= STAMINA_RUN_DRAIN_INTERVAL):      │
   │         drainStamina(STAMINA_RUN_DRAIN)                         │
   │         staminaDrainAccum = 0                                   │
   │     staminaRegenAccum = 0  // No regen while running           │
   │                                                                 │
   │ else:  // IDLE or WALK — passive regen                         │
   │     staminaDrainAccum = 0                                       │
   │     staminaRegenAccum++                                         │
   │     if (staminaRegenAccum >= STAMINA_REGEN_INTERVAL):          │
   │         uint8_t regenAmt = (moveTier == MOVE_IDLE)             │
   │             ? STAMINA_REGEN_IDLE                                │
   │             : STAMINA_REGEN_WALK;                               │
   │         recoverStamina(regenAmt)                                │
   │         staminaRegenAccum = 0                                   │
   │                                                                 │
   │ // Force tier downgrade if stamina depleted mid-action         │
   │ if (stamina == 0 && moveTier == MOVE_SPRINT):                  │
   │     moveTier = MOVE_RUN  // or WALK if < STAMINA_MIN_TO_RUN   │
   │ if (stamina < STAMINA_MIN_TO_RUN && moveTier == MOVE_RUN):    │
   │     moveTier = MOVE_WALK                                        │
   └─────────────────────────────────────────────────────────────────┘

STEP 8: Directional movement & inertia blending (RESTRUCTURED)
   - Walk tier: apply movement even without A button
     (current code only moves when accel is true — change this)
   - The `if (hasDir)` block should use the tier's maxSpd and accelVal
     instead of checking `if (accel)` for thrust
   - Inertia blend factor stays at PLAYER_INERTIA (0.25)

STEP 9: Deceleration / friction (RESTRUCTURED — BRAKE REMOVED)
   - BRAKE BEHAVIOR REMOVED: Delete the `if (brake) { ... }` branch entirely.
   - Natural deceleration: when no directional input is active (`!hasDir`), apply natural foot friction (`PLAYER_FRICTION`) toward zero velocity (stops in ~20 frames):
     ```cpp
     if (!hasDir) {
         fp_t fStep = FP_MUL(PLAYER_FRICTION, dt);
         if (vx > 0) { vx -= fStep; if (vx < 0) vx = 0; }
         if (vx < 0) { vx += fStep; if (vx > 0) vx = 0; }
         if (vy > 0) { vy -= fStep; if (vy < 0) vy = 0; }
         if (vy < 0) { vy += fStep; if (vy > 0) vy = 0; }
     }
     ```
   - When directional input is pressed (`hasDir`), velocity is continuously steered and blended toward the tier's target velocity via `PLAYER_INERTIA`.

STEP 10: Circular velocity clamping (UNCHANGED)

STEP 11: Position integration (UNCHANGED)

STEP 12: Walk animation cadence (ADJUSTED by tier)
   - MOVE_WALK:   animDivisor = PLAYER_WALK_ANIM_DIVISOR * 2  (slow steps)
   - MOVE_RUN:    animDivisor = PLAYER_WALK_ANIM_DIVISOR       (normal)
   - MOVE_SPRINT: animDivisor = max(2, PLAYER_WALK_ANIM_DIVISOR / 2)  (fast steps)
```

##### `Player::applyFoodSlow()` — Add stamina drain

Add `drainStamina()` call inside each `case` block, after setting `slowTimer`/`slowIntensity`:

```cpp
case ENTITY_FOOD_APPLE:
    slowTimer = FOOD_APPLE_SLOW_DURATION;
    slowIntensity = FOOD_APPLE_SLOW_INTENSITY;
    drainStamina(STAMINA_FOOD_APPLE_DRAIN);    // NEW
    break;
case ENTITY_FOOD_PIZZA:
    slowTimer = FOOD_PIZZA_SLOW_DURATION;
    slowIntensity = FOOD_PIZZA_SLOW_INTENSITY;
    drainStamina(STAMINA_FOOD_PIZZA_DRAIN);    // NEW
    break;
// ... same pattern for TACO, BURGER, FRIES, CAKE, DONUT, ICECREAM
```

##### `Player::applyCoffeeBoost()` — Add stamina recovery

```cpp
void Player::applyCoffeeBoost() {
    boostTimer = POWERUP_COFFEE_DURATION;
    boostTimerAccum = 0;
    slowTimer = 0;
    slowTimerAccum = 0;
    recoverStamina(STAMINA_COFFEE_RECOVERY);  // NEW: coffee restores stamina
}
```

##### New helper methods

```cpp
void Player::drainStamina(uint8_t amount) {
    if (stamina > amount) stamina -= amount;
    else stamina = 0;
}

void Player::recoverStamina(uint8_t amount) {
    uint16_t sum = (uint16_t)stamina + amount;
    stamina = (sum > STAMINA_MAX) ? STAMINA_MAX : (uint8_t)sum;
}

uint8_t Player::getStaminaPercent() const {
    return (uint8_t)(((uint16_t)stamina * 100) / STAMINA_MAX);
}

MoveTier Player::getCurrentMoveTier() const {
    return moveTier;
}
```

---

### Component 3: Game Integration

#### [MODIFY] [game.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.h)

**Add cooldown field** to the `Game` class (after `comboMultiplier`, ~line 86):

```cpp
uint8_t gameOverCooldown;  // Frames remaining before accepting input on game-over screen
```

No other structural changes needed — `game.h` already includes `player.h`, so the `MoveTier` enum is visible.

---

#### [MODIFY] [game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp)

##### `updatePlaying()` — Pass `justPressedA` to player (~line 123-133)

```diff
 void Game::updatePlaying(HalInput& input, fp_t dt) {
     // 1. Read input
     bool up    = input.pressed(BTN_UP);
     bool down  = input.pressed(BTN_DOWN);
     bool left  = input.pressed(BTN_LEFT);
     bool right = input.pressed(BTN_RIGHT);
     bool accel = input.pressed(BTN_A);
+    bool justA = input.justPressed(BTN_A);
-    bool brake = input.pressed(BTN_B);

     // 2. Update player movement with delta time
-    player.update(up, down, left, right, accel, brake, dt);
+    player.update(up, down, left, right, accel, justA, dt);
```

> [!IMPORTANT]
> `BTN_B` is removed from `updatePlaying()`. However, `BTN_B` MUST remain in `updateGameOver()` to handle the "Retry" action (`reset(); state = STATE_PLAYING;`).

##### Game-over transition — Set cooldown (~line 297-307)

When entering `STATE_GAMEOVER`, initialize the cooldown timer:

```diff
     if (checkOverlap(player.x, player.y, player.width, player.height,
                       world.bhX, world.bhY,
                       BH_RENDER_RADIUS * 2, BH_RENDER_RADIUS * 2)) {
         state = STATE_GAMEOVER;
+        gameOverCooldown = GAMEOVER_COOLDOWN_FRAMES;  // Prevent residual A-tap input
         if (score > highScore) {
             highScore = score;
             if (storageRef) {
                 storageRef->saveHighScore(highScore);
             }
         }
     }
```

##### `updateGameOver()` — Enforce cooldown before accepting input (~line 323-331)

```diff
 void Game::updateGameOver(HalInput& input, fp_t dt) {
     (void)dt;
+    // Cooldown: ignore all input for GAMEOVER_COOLDOWN_FRAMES after death.
+    // Prevents accidental menu selection from residual sprint-tapping.
+    if (gameOverCooldown > 0) {
+        gameOverCooldown--;
+        return;  // No input processing during cooldown
+    }
     if (input.justPressed(BTN_A)) {
         state = STATE_TITLE;
     } else if (input.justPressed(BTN_B)) {
         reset();
         state = STATE_PLAYING;
     }
 }
```

##### `renderGameOver()` — Visual cooldown indicator (~line 333-352)

Optionally show a brief "loading" or dimmed state during cooldown:

```diff
 void Game::renderGameOver(HalRenderer& renderer) {
+    // During cooldown, show a brief "settling" animation (dimmed or delayed text)
+    if (gameOverCooldown > 0) {
+        renderer.setCursor(37, 8);
+        renderer.print("GAME OVER");
+        renderer.setCursor(22, 24);
+        renderer.print("Score:");
+        renderer.printNumber(score);
+        renderer.setCursor(22, 36);
+        renderer.print("High:");
+        renderer.printNumber(highScore);
+        // No button prompts shown during cooldown — player knows to wait
+        return;
+    }
+
     renderer.setCursor(37, 8);
     renderer.print("GAME OVER");
     // ... (rest unchanged, button prompts only appear after cooldown)
```

##### `renderHUD()` — Add stamina bar (~line 554, after existing HUD box)

```cpp
// === STAMINA BAR (vertical, right edge) ===
// Outer border
renderer.drawRect(STAMINA_BAR_X, STAMINA_BAR_Y,
                  STAMINA_BAR_WIDTH, STAMINA_BAR_HEIGHT, COLOR_WHITE);

// Inner fill (bottom-up, proportional to current stamina)
uint8_t fillHeight = (uint8_t)(((uint16_t)player.stamina * (STAMINA_BAR_HEIGHT - 2)) / STAMINA_MAX);
if (fillHeight > 0) {
    int16_t fillY = STAMINA_BAR_Y + (STAMINA_BAR_HEIGHT - 1) - fillHeight;
    renderer.fillRect(STAMINA_BAR_X + 1, fillY,
                      STAMINA_BAR_WIDTH - 2, fillHeight, COLOR_WHITE);
}

// Blink entire bar when stamina is critically low
if (player.stamina < STAMINA_CRITICAL_THRESHOLD && (world.gameTime / 8) % 2 == 0) {
    renderer.fillRect(STAMINA_BAR_X, STAMINA_BAR_Y,
                      STAMINA_BAR_WIDTH, STAMINA_BAR_HEIGHT, COLOR_BLACK);
}
```

##### `renderPlayer()` — Add sprint visual effects (~line 507, after existing boost trail)

```cpp
// Sprint visual effect: extra motion trail particles (more intense than boost trail)
if (player.getCurrentMoveTier() == MOVE_SPRINT && (player.vx != 0 || player.vy != 0)) {
    int16_t trailX = sx - FP_TO_INT(player.vx * 5);
    int16_t trailY = sy - FP_TO_INT(player.vy * 5);
    if ((world.gameTime % 2) == 0) {
        renderer.drawPixel(trailX - 3, trailY - 1, COLOR_WHITE);
        renderer.drawPixel(trailX + 3, trailY + 1, COLOR_WHITE);
        renderer.drawPixel(trailX - 1, trailY - 3, COLOR_WHITE);
        renderer.drawPixel(trailX + 1, trailY + 3, COLOR_WHITE);
    }
}
```

##### `reset()` — Initialize cooldown

```diff
 void Game::reset() {
     player.init();
     world.init();
     entities.init();
     score = 0;
     comboMultiplier = 1;
+    gameOverCooldown = 0;
```

---

## File Change Summary

| File | Change | Size Impact |
|------|--------|-------------|
| [`config.h`](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/config.h) | Remove `PLAYER_BRAKE_FRICTION`. Add ~70 lines: speed tiers, sprint detection, stamina system, stamina bar HUD, game-over cooldown constants | Code only (no RAM) |
| [`player.h`](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/player.h) | Add `MoveTier` enum, ~10 new fields, remove `brake` from `update()` signature, add 4 new methods | +10 bytes RAM |
| [`player.cpp`](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/player.cpp) | Rewrite `update()` with sprint tap FSM + stamina. Remove brake branch (natural friction on `!hasDir`). Modify `init()`, `applyCoffeeBoost()`, `applyFoodSlow()`. Add 4 helper methods | ~+115 lines |
| [`game.h`](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.h) | Add `gameOverCooldown` field | +1 byte RAM |
| [`game.cpp`](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp) | Remove `brake` input reading in `updatePlaying()`, pass `justPressedA` to player. Add cooldown logic to game-over. Add stamina bar to HUD. Add sprint trail to player render | ~+48 lines |

**Total RAM impact: ~11 bytes** (well within ~2100 byte headroom)

---

## Verification Plan

### Automated Tests

Build verification:
```bash
cd /home/dorito/Developer/arduboy/supermassive-whitehole && bash build.sh
```

### Manual Verification Checklist

| # | Test | Expected Result |
|---|------|-----------------|
| 1 | **Walk** — Press D-pad only, no A | Player moves slowly (~40% speed). Stamina bar shows full. No drain. |
| 2 | **Run** — Hold D-pad + A | Player runs at ~75% speed. Stamina bar drains slowly. |
| 3 | **Sprint (slow taps)** — Tap A moderately while D-pad held | Speed slightly above run. Variable speed based on tap interval. |
| 4 | **Sprint (fast taps)** — Tap A rapidly while D-pad held | Player sprints at near-max speed. Stamina drains fast. |
| 5 | **Sprint depletion** — Sprint until empty | Forced downgrade to run, then walk. Stamina bar blinks. |
| 6 | **Idle regen** — Stand still after depleting stamina | Stamina recovers ~32 sec from empty. Bar fills from bottom. |
| 7 | **Walk regen** — Walk after depleting stamina | Stamina recovers ~64 sec from empty (slower than idle). |
| 8 | **No regen while running** — Run continuously | Stamina drains, does NOT recover even if speed is moderate. |
| 9 | **Coffee pickup** — Collect coffee powerup | Stamina jumps up by ~30%. Speed boost still works. Slow cleared. |
| 10 | **Food pickup** — Touch food hazard | Stamina drops (varies by food). Speed slow still applies. |
| 11 | **Game-over cooldown** — Die while sprint-tapping | "GAME OVER" shows without button prompts for ~1.5 sec. No accidental A selection. |
| 12 | **Post-cooldown menu** — Wait on game-over screen | After cooldown, "A:Menu B:Retry" appears. Buttons work normally. |
| 13 | **Title screen** — Navigate to title from game-over | A button works as expected (no sprint interference). |
| 14 | **Stamina bar HUD** — Visual check | 3px wide bar on right edge, fills/drains smoothly, blinks when <15%. |
| 15 | **Sprint trail VFX** — Sprint past entities | Extra motion particles appear behind player. |
| 16 | **Walk vs blackhole** — Walk-only at game start | Player can BARELY outrun the initial blackhole (BH_BASE_SPEED=0.20 vs WALK=0.38). |
| 17 | **No Brake / Natural Coasting** — Release D-pad while moving; press B during gameplay | Pressing/holding B during gameplay does nothing. Releasing D-pad brings player to a natural coasting stop via `PLAYER_FRICTION`. |
| 18 | **B Button Menu Navigation** — Press B on Game Over screen after cooldown | Triggers game Retry (`reset()`), verifying B button remains active for UI navigation. |

### Tuning Guidelines

> [!TIP]
> These values need playtesting. Adjust in `config.h` — all tuning constants are centralized there.

- **Sprint duration:** Should last **5–7 seconds** of sustained max-speed tapping before depletion
- **Full regen (idle):** Should take **25–35 seconds** from empty
- **Full regen (walk):** Should take **55–70 seconds** from empty
- **Walk speed:** Must feel noticeably slow but not frustrating; player should WANT to run
- **Run speed:** Should feel like the "comfortable default" pace
- **Sprint max speed:** Should feel like a powerful burst — impactful and costly
- **Tap cadence for max sprint:** ~4–5 taps/second (tap interval of 12–15 frames)
- **Game-over cooldown:** 1.0–2.0 seconds — enough to feel deliberate, not annoying
- **Coffee recovery:** Should feel like a meaningful boost (~30%) but not fully restore
- **Food stamina drain:** Heavy foods (cake, ice cream) should be punishing; light foods (apple) negligible
