// =============================================================================
// config.h — Game-Wide Constants & Fixed-Point Math
// =============================================================================
//
// PURPOSE:
//   Central configuration file for ALL game constants, tuning parameters,
//   and fixed-point arithmetic helpers. Every game logic file includes this.
//   This file MUST remain platform-independent (no Arduino/Arduboy headers).
//
// FIXED-POINT MATH (Q8.8):
//   The Arduboy's ATmega32u4 has NO floating-point unit. All "decimal" math
//   uses Q8.8 fixed-point: an int16_t where the upper 8 bits are the integer
//   part and the lower 8 bits are the fractional part.
//
//   Example: 1.5 in Q8.8 = 1 * 256 + 0.5 * 256 = 384
//   Example: -2.25 in Q8.8 = -576
//
//   Use INT_TO_FP() to convert integers, FP_MUL() for multiplication,
//   FP_TO_INT() to extract the integer part for screen coordinates.
//
// MEMORY BUDGET:
//   Total SRAM: 2560 bytes. Stack needs ~200-300 bytes.
//   Entity array: MAX_ENTITIES * sizeof(Entity) ≈ 12 * 14 = 168 bytes
//   Player: ~30 bytes
//   World: ~40 bytes
//   Game: ~20 bytes
//   Remaining for stack + locals: ~2100 bytes (comfortable)
//
// FOR IMPLEMENTING AGENTS:
//   - Adjust physics constants (PLAYER_ACCEL, FRICTION, etc.) during M1
//     playtesting to get the "heavy, drifty" feel right
//   - BLACKHOLE constants ramp linearly with gameTime; tune during M3
//   - Food slow values are in frames (at 60 FPS); tune during M2
//   - If RAM is tight, reduce MAX_ENTITIES first
//
// =============================================================================

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// =============================================================================
// PLATFORM SELECTOR
// =============================================================================
// Define the target platform here. When porting to a new platform (e.g.,
// Raylib), add a new #define and update src/hal/hal_types.h accordingly. Only
// ONE platform should be defined at a time.

#ifndef PLATFORM_ARDUBOY
#define PLATFORM_ARDUBOY
#endif

// =============================================================================
// SCREEN DIMENSIONS
// =============================================================================
// Arduboy: 128x64 pixels, 1-bit (monochrome: black=0, white=1)
// These are used for screen-space calculations, HUD placement, and spawn
// bounds.

static const uint8_t SCREEN_W = 128;
static const uint8_t SCREEN_H = 64;

// =============================================================================
// FIXED-POINT ARITHMETIC (Q8.8)
// =============================================================================
// All positions, velocities, and physics calculations use Q8.8 fixed-point.
// This gives sub-pixel precision (1/256 pixel) while using only int16_t.
//
// IMPORTANT: Multiplication of two Q8.8 values requires a 32-bit intermediate
// to avoid overflow, then shifting back. Always use FP_MUL(), never raw *.
//
// RANGE: Q8.8 int16_t ranges from -128.0 to +127.996 (~128).
//        For world coordinates that exceed this range, use int32_t (fp32_t).

#define FP_SHIFT 8
#define FP_ONE (1 << FP_SHIFT)        // = 256, represents 1.0
#define FP_HALF (1 << (FP_SHIFT - 1)) // = 128, represents 0.5

// Type aliases for clarity
typedef int16_t fp_t;   // Q8.8 fixed-point (range: -128 to ~+128)
typedef int32_t fp32_t; // Q16.8 or Q24.8 for world coordinates (large range)

// Convert integer to fixed-point: e.g., INT_TO_FP(3) = 768
#define INT_TO_FP(x) ((fp_t)((int16_t)(x) << FP_SHIFT))

// Convert fixed-point to integer (truncates fractional part): e.g.,
// FP_TO_INT(768) = 3
#define FP_TO_INT(x) ((int16_t)((x) >> FP_SHIFT))

// Convert float to fixed-point at compile time ONLY (for initializing
// constants): e.g., FLOAT_TO_FP(1.5) = 384. NEVER use at runtime — no FPU!
#define FLOAT_TO_FP(f) ((fp_t)((f) * FP_ONE))

// Multiply two Q8.8 values: uses 32-bit intermediate to prevent overflow.
// Example: FP_MUL(INT_TO_FP(2), INT_TO_FP(3)) = INT_TO_FP(6)
#define FP_MUL(a, b) ((fp_t)(((int32_t)(a) * (int32_t)(b)) >> FP_SHIFT))

// Divide two Q8.8 values: shifts numerator up first for precision.
// Example: FP_DIV(INT_TO_FP(6), INT_TO_FP(2)) = INT_TO_FP(3)
#define FP_DIV(a, b) ((fp_t)(((int32_t)(a) << FP_SHIFT) / (int32_t)(b)))

// Absolute value for fixed-point
#define FP_ABS(x) ((x) < 0 ? -(x) : (x))

// =============================================================================
// LARGE WORLD COORDINATES (Q24.8 using int32_t)
// =============================================================================
// The infinite plane requires world coordinates larger than Q8.8's ±128 range.
// Player position, camera, and blackhole use fp32_t (int32_t with 8-bit
// fraction). Same shift factor — all FP macros work, but use FP32 variants for
// 32-bit ops.

#define INT_TO_FP32(x) ((fp32_t)((int32_t)(x) << FP_SHIFT))
#define FP32_TO_INT(x) ((int32_t)((x) >> FP_SHIFT))
#define FP32_MUL(a, b) ((fp32_t)(((int64_t)(a) * (int64_t)(b)) >> FP_SHIFT))

// =============================================================================
// FRAME RATE
// =============================================================================

static const uint8_t TARGET_FPS = 60;

// Delta-time normalization: 1.0 in Q8.8 represents exactly one 60 FPS frame
// (16.67ms)
#define FP_DT_ONE FP_ONE

// =============================================================================
// ANIMATION TIMING CONSTANTS
// =============================================================================
// Divisors to convert frame/spin ticks into sprite animation frames.
// Higher divisor = slower, smoother, more deliberate animation.
static const uint8_t WHITEHOLE_ANIM_DIVISOR =
    12; // 60 FPS / 12 = 5 FPS sprite animation (~0.8s per revolution)
static const uint8_t PLAYER_BLINK_DIVISOR =
    8; // 60 FPS / 8 = 7.5 Hz debuff blink cadence
static const uint8_t PLAYER_WALK_ANIM_DIVISOR =
    8; // 60 FPS / 8 = 7.5 Hz walk step cadence

// =============================================================================
// PLAYER PHYSICS CONSTANTS (HUMAN ON FOOT - AGILE STEERING & SMALL IMPULSE)
// =============================================================================
// The player is a fat man on foot, NOT a wheeled vehicle:
//   - D-pad steers agilely with high responsiveness (humans turn on a dime)
//   - A button (gas/walk) delivers a small grounded impulse per stride
//   - Small inertia feedback (0.25) gives body weight without car-like sliding
//   - Natural foot friction (0.040) stops in a few strides when coasting
//   - Dedicated brake behavior removed (outdated with GTA:SA locomotion)
//
// Tuning parameters:
//   PLAYER_STEP_IMPULSE: Initial stride impulse from standstill (0.25 px/frame)
//   PLAYER_ACCEL: Continuous stride impulse per frame while A held (0.040 px/frame)
//   PLAYER_FRICTION: Foot drag when coasting (0.040 px/frame, stops in ~20 frames)
//   PLAYER_MAX_SPEED: Comfortable human jogging pace (0.95 px/frame ~= 57 px/sec)
//   PLAYER_INERTIA: Steering blend factor (0.25 = 75% instant turn, 25% weight)
//   DIAGONAL_FACTOR: 1/sqrt(2) in Q8.8 (181/256 ~= 0.7071) for equal 8-dir speed

static const fp_t PLAYER_STEP_IMPULSE =
    FLOAT_TO_FP(0.25);                               // Initial stride impulse
static const fp_t PLAYER_ACCEL = FLOAT_TO_FP(0.040); // Stride impulse per frame
static const fp_t PLAYER_FRICTION = FLOAT_TO_FP(0.040); // Natural foot friction
static const fp_t PLAYER_MAX_SPEED = FLOAT_TO_FP(0.95); // Max jogging speed
static const fp_t PLAYER_INERTIA =
    FLOAT_TO_FP(0.25);                   // Agile human steering (small inertia)
static const fp_t DIAGONAL_FACTOR = 181; // 1/sqrt(2) in Q8.8 (~0.7071)

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
// Sprint is activated by tapping A rapidly (release -> re-press within window).
// Sprint SPEED scales with tap FREQUENCY — faster taps = faster sprint.
//
// State machine: track frames between consecutive A presses (tapInterval).
//   tapInterval <  SPRINT_TAP_WINDOW -> sprinting (speed proportional to 1/tapInterval)
//   tapInterval >= SPRINT_TAP_WINDOW -> not sprinting (just running if A held)
//
// A minimum of SPRINT_TAP_COUNT_NEEDED consecutive fast taps is required
// before entering sprint mode (prevents accidental single fast press).
static const uint8_t SPRINT_TAP_WINDOW       = 15; // Max frames between A presses to count as sprint tap (~250ms at 60 FPS)
static const uint8_t SPRINT_TAP_COUNT_NEEDED = 2;  // Fast taps needed before sprint activates
static const uint8_t SPRINT_SUSTAIN_WINDOW   = 20; // If A is HELD (not tapped) for this many frames, downgrade to run

// =============================================================================
// STAMINA SYSTEM CONSTANTS
// =============================================================================
// Stamina is a uint8_t (0-255) representing the player's endurance pool.
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
// Run:    drain 1 point every 10 frames -> ~42 sec to deplete from full
// Sprint: drain 3 points every 4 frames -> ~5.7 sec to deplete from full
static const uint8_t STAMINA_RUN_DRAIN_INTERVAL    = 10;
static const uint8_t STAMINA_SPRINT_DRAIN_INTERVAL  = 4;

// Regen tick values and interval
// Idle:  regen 2 every 15 frames -> ~32 sec to fully recover
// Walk:  regen 1 every 15 frames -> ~64 sec to fully recover
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
static const uint8_t STAMINA_BAR_Y       = 16;   // Y position (below score box)
static const uint8_t STAMINA_BAR_WIDTH   = 3;    // Width in pixels
static const uint8_t STAMINA_BAR_HEIGHT  = 44;   // Height in pixels
static const uint8_t STAMINA_CRITICAL_THRESHOLD = 38;  // ~15% — bar blinks below this

// =============================================================================
// GAME-OVER COOLDOWN CONSTANTS
// =============================================================================
// Brief no-input cooldown screen after game over, before showing title/menu.
// Prevents accidental menu selection from residual sprint-tapping when the
// player dies while frantically tapping A to escape the whitehole.
static const uint8_t GAMEOVER_COOLDOWN_FRAMES = 90;  // ~1.5 sec at 60 FPS

// Player hitbox size in pixels (used for collision detection)
static const uint8_t PLAYER_WIDTH = 10;
static const uint8_t PLAYER_HEIGHT = 10;

// Player starting position offset from blackhole (in world units)
static const int16_t PLAYER_START_OFFSET = 60;

// =============================================================================
// ENTITY SYSTEM CONSTANTS
// =============================================================================
// MAX_ENTITIES is the static array size for all on-screen entities (food +
// collectibles). The blackhole is NOT in this array — it's managed by World.
//
// Each Entity is ~14 bytes. 12 entities = 168 bytes of RAM.
// Increase cautiously — every +1 costs ~14 bytes.

static const uint8_t MAX_ENTITIES = 12;

// Entity sizes (pixels) — used for collision hitboxes (centered within 16x16
// sprites) Food hazards have slightly forgiving hitboxes (8px) for near-miss
// excitement. Collectibles & power-ups have generous hitboxes (10px) for
// satisfying pickups.
static const uint8_t FOOD_APPLE_SIZE = 8;
static const uint8_t FOOD_PIZZA_SIZE = 8;
static const uint8_t FOOD_TACO_SIZE = 8;
static const uint8_t FOOD_BURGER_SIZE = 8;
static const uint8_t FOOD_FRIES_SIZE = 8;
static const uint8_t FOOD_CAKE_SIZE = 8;
static const uint8_t FOOD_DONUT_SIZE = 8;
static const uint8_t FOOD_ICECREAM_SIZE = 8;
static const uint8_t COLLECTIBLE_SIZE = 10;
static const uint8_t POWERUP_COFFEE_SIZE = 10;

// =============================================================================
// POWER-UP CONSTANTS
// =============================================================================
// Coffee provides an instant caffeine rush: cleanses food slows and grants
// a speed and acceleration burst for 3 seconds (180 frames at 60 FPS).
static const uint8_t POWERUP_COFFEE_DURATION = 180; // ~3.0 sec
static const fp_t POWERUP_COFFEE_SPEED_BOOST =
    FLOAT_TO_FP(1.35); // +35% max speed
static const fp_t POWERUP_COFFEE_ACCEL_BOOST =
    FLOAT_TO_FP(1.50); // +50% acceleration

// =============================================================================
// FOOD SLOWDOWN CONSTANTS (8 DISTINCT HAZARDS)
// =============================================================================
// When the fat man touches food, he's temporarily slowed.
// Duration is in frames (at 60 FPS): 30 frames = 0.5 seconds.
// Intensity is a percentage of speed REDUCTION (0-100).
//
// Diverse hazards ranging from quick snacks to heavy feasts:
static const uint8_t FOOD_APPLE_SLOW_DURATION = 24;  // ~0.4 sec
static const uint8_t FOOD_APPLE_SLOW_INTENSITY = 10; // -10% speed

static const uint8_t FOOD_PIZZA_SLOW_DURATION = 30;  // ~0.5 sec
static const uint8_t FOOD_PIZZA_SLOW_INTENSITY = 15; // -15% speed

static const uint8_t FOOD_TACO_SLOW_DURATION = 45;  // ~0.75 sec
static const uint8_t FOOD_TACO_SLOW_INTENSITY = 25; // -25% speed

static const uint8_t FOOD_BURGER_SLOW_DURATION = 60;  // ~1.0 sec
static const uint8_t FOOD_BURGER_SLOW_INTENSITY = 30; // -30% speed

static const uint8_t FOOD_FRIES_SLOW_DURATION = 75;  // ~1.25 sec
static const uint8_t FOOD_FRIES_SLOW_INTENSITY = 35; // -35% speed

static const uint8_t FOOD_CAKE_SLOW_DURATION = 105; // ~1.75 sec
static const uint8_t FOOD_CAKE_SLOW_INTENSITY = 45; // -45% speed

static const uint8_t FOOD_DONUT_SLOW_DURATION = 90;  // ~1.5 sec
static const uint8_t FOOD_DONUT_SLOW_INTENSITY = 50; // -50% speed

static const uint8_t FOOD_ICECREAM_SLOW_DURATION =
    120; // ~2.0 sec (Brain freeze!)
static const uint8_t FOOD_ICECREAM_SLOW_INTENSITY = 60; // -60% speed

// =============================================================================
// BLACKHOLE CONSTANTS
// =============================================================================
// The blackhole is a single entity that tracks the player's position.
// It translates the 3 real blackhole properties:
//   - Mass:   gravitational pull strength on nearby entities
//   - Spin:   visual rotation effect (cosmetic, uint8_t angle counter)
//   - Charge: attraction radius — how far its gravity reaches
//
// The blackhole starts slow and far away. Over time (linear ramp):
//   - Speed increases → harder to outrun
//   - Charge increases → harder to keep distance
//   - Mass stays constant (pull strength within charge radius is fixed)
//
// Game over when blackhole hitbox overlaps player hitbox.
//
// FOR IMPLEMENTING AGENTS (M3):
//   The blackhole moves toward the player each frame:
//     direction = normalize(player.pos - blackhole.pos)
//     blackhole.pos += direction * bhSpeed
//   bhSpeed increases by BH_SPEED_INCREMENT every frame.
//   Use physics.h moveToward() helper.

static const fp_t BH_BASE_SPEED =
    FLOAT_TO_FP(0.20);                    // Initial chase speed (~12 px/sec)
static const fp_t BH_SPEED_INCREMENT = 1; // Speed increase step in Q8.8 (1/256)
static const uint8_t BH_SPEED_RAMP_INTERVAL =
    120; // Frames between speed increases (~2 sec)
static const fp_t BH_MASS = FLOAT_TO_FP(1.0); // Gravity pull strength (fixed)
static const fp_t BH_BASE_CHARGE =
    INT_TO_FP(30);                         // Initial attraction radius (pixels)
static const fp_t BH_CHARGE_INCREMENT = 1; // Charge increase step in Q8.8
static const uint8_t BH_CHARGE_RAMP_INTERVAL =
    120; // Frames between charge increases (~2 sec)

// Blackhole visual size (pixels) — concentric circles, larger than entities
static const uint8_t BH_RENDER_RADIUS = 8;

// Starting distance from player (in world-space pixels)
static const int16_t BH_START_DISTANCE = 80;

// =============================================================================
// SPAWNING CONSTANTS
// =============================================================================
// Entities spawn around the player, ahead of movement direction.
// Spawn interval decreases linearly over time (more food/collectibles).
//
// FOR IMPLEMENTING AGENTS (M2):
//   Every SPAWN_INTERVAL frames, attempt to spawn one entity.
//   Choose a random position within SPAWN_RADIUS of the player, but
//   at least SPAWN_MIN_DISTANCE away (so they don't appear on top of player).
//   Entity type is chosen randomly with weighted probabilities:
//     ~60% food (equal split pizza/burger/donut), ~40% collectible.
//   Spawn interval decreases by 1 every SPAWN_RAMP_INTERVAL frames,
//   down to SPAWN_MIN_INTERVAL.

static const uint8_t SPAWN_INTERVAL_START =
    180; // Frames between spawns (initial ~3 sec for clean readability)
static const uint8_t SPAWN_MIN_INTERVAL =
    60; // Minimum spawn interval (~1.0 sec)
static const uint8_t SPAWN_RAMP_INTERVAL =
    240; // Frames between spawn rate increases (~4 sec)
static const uint8_t SPAWN_RADIUS =
    120; // Max distance from player to spawn (generous spread)
static const uint8_t SPAWN_MIN_DISTANCE =
    55; // Min distance from player to spawn (well ahead)
static const uint8_t MIN_ITEM_SEPARATION =
    45; // Minimum world distance between any two items
static const uint8_t DESPAWN_DISTANCE =
    130; // Distance from camera edge to despawn

// =============================================================================
// MONEY DISTRIBUTION CONSTANTS (WHITEHOLE RISK-REWARD ZONE)
// =============================================================================
// Money collectibles (Diamond and Bills) spawn primarily in an orbit around
// the whitehole. The spawn distance follows a decaying distribution curve
// that peaks just outside the whitehole's attraction radius (charge).
//
//   minSafeDist = FP_TO_INT(bhCharge) + MONEY_BH_SAFE_BUFFER
//
// This guarantees that neither the money nor the player gets eaten by the
// whitehole's attraction force (M3) when grabbing the treasure.
static const uint8_t MONEY_BH_SPAWN_CHANCE =
    80; // % of money spawns targeting the whitehole orbit
static const uint8_t MONEY_BH_SAFE_BUFFER =
    20; // Buffer px beyond bhCharge (ensures player clearance)
static const uint8_t MONEY_BH_RING_SPAN =
    40; // Width of orbit ring for decaying distribution

// 16-point unit circle lookup table (normalized to 127) for isotropic radial
// placement
static const int8_t UNIT_CIRCLE_X[16] = {
    127, 117, 90, 49, 0, -49, -90, -117, -127, -117, -90, -49, 0, 49, 90, 117};
static const int8_t UNIT_CIRCLE_Y[16] = {
    0, 49, 90, 117, 127, 117, 90, 49, 0, -49, -90, -117, -127, -117, -90, -49};

// =============================================================================
// SCORING
// =============================================================================
// Points for collectibles and passive survival time.

static const uint8_t SCORE_PER_DIAMOND = 10; // Diamond gem (+10 points)
static const uint8_t SCORE_PER_BILLS = 25; // Stack of dollar bills (+25 points)
static const uint8_t SCORE_PER_COLLECTIBLE = 10; // Default / fallback
static const uint8_t COMBO_MAX =
    4; // Max score multiplier from streaks (1x..4x)
static const uint8_t SCORE_TIME_INTERVAL =
    60;                                  // Frames between passive score ticks
static const uint8_t SCORE_PER_TICK = 1; // Points per time tick

// =============================================================================
// EEPROM
// =============================================================================
// Address in EEPROM to store the high score (uint16_t = 2 bytes).
// Arduboy reserves EEPROM 0-15 for system use. Start at 16.

static const uint16_t EEPROM_HIGH_SCORE_ADDR = 16;

// =============================================================================
// PERSPECTIVE GROUND PLANE
// =============================================================================
// Pseudo-3D ground grid with quadratic depth foreshortening.
// The vanishing point (HORIZON_Y) is placed ABOVE the screen top for a
// dramatic perspective where the full 128×64 display is ground plane.
// Objects recede toward the top of the screen and naturally scroll off —
// no visible horizon line means nothing can "fly above" it.
//
// CUSTOMIZATION GUIDE (adjust these to change the camera angle):
//   HORIZON_Y:  Lower = more dramatic convergence. 0 = horizon at screen top.
//               Negative = vanishing point above screen (recommended: -10 to
//               -20). Positive = visible horizon line on screen.
//   GROUND_DEPTH_PLAYER: Player's Z-depth on the plane. Controls vertical
//               position on screen. Higher = player renders lower.
//   PERSPECTIVE_MAX_Z: Z-depth at screen bottom edge. Controls the depth range.
//   BASE_SPACING_X / TOP_SPACING_X: Control perspective ray convergence.
//               Ratio TOP/BASE determines how much rays converge at the
//               vanishing point.
//   Z_PERIOD: Distance between horizontal depth lines. Smaller = more lines
//   visible.
static const int16_t HORIZON_Y = -10; // Vanishing point Y (off-screen top)
static const int16_t GROUND_HEIGHT =
    74; // SCREEN_H - HORIZON_Y (ground span px)
static const int16_t GROUND_DEPTH_PLAYER = 91; // Player Z-depth (screen Y ≈ 32)
static const int16_t PERSPECTIVE_MAX_Z = 120;  // Z at screen bottom edge
static const int32_t PERSPECTIVE_MAX_Z_SQ =
    14400;                                // PERSPECTIVE_MAX_Z^2 (precomputed)
static const int16_t BASE_SPACING_X = 24; // Ray spacing at screen bottom
static const int16_t TOP_SPACING_X = 6;   // Ray spacing at vanishing point
static const int16_t Z_PERIOD = 24;       // Depth line period

// =============================================================================
// HUD POSITION
// =============================================================================
// Configurable position for the score / HUD overlay.
// A black background rectangle is drawn behind the text for readability
// over the grid lines (since there is no open sky area).
enum HudPosition : uint8_t {
  HUD_TOP_RIGHT = 0,
  HUD_TOP_LEFT = 1,
  HUD_BOTTOM_RIGHT = 2,
  HUD_BOTTOM_LEFT = 3
};
static const HudPosition HUD_POSITION = HUD_TOP_RIGHT;

// =============================================================================
// COLORS (1-bit: 0=BLACK, 1=WHITE)
// =============================================================================
// Named constants for clarity in rendering code.

static const uint8_t COLOR_BLACK = 0;
static const uint8_t COLOR_WHITE = 1;

// =============================================================================
// SPRITE RENDERING CONFIGURATION (TRANSPARENCY & OUTLINE)
// =============================================================================
// Configures transparency (1-bit alpha / opaque background) and outline styles
// for entities, player, and whitehole.
//
// 1. Transparency / 1-bit Alpha Mode
//    - SPRITE_ALPHA_OPAQUE:      Solid black background fills sprite
//    silhouette.
//                                Blocks background grid lines from bleeding
//                                through.
//    - SPRITE_ALPHA_TRANSPARENT: Unmasked / transparent background.
//                                Background grid lines show through 0-bits.
enum SpriteAlphaMode : uint8_t {
  SPRITE_ALPHA_TRANSPARENT = 0,
  SPRITE_ALPHA_OPAQUE = 1,
};

// 2. Outline Mode (always uses solid black background when active)
//    - SPRITE_OUTLINE_NONE:  No outline border (silhouette mask only if
//    opaque).
//    - SPRITE_OUTLINE_BLACK: Black outline border halo (cuts background grid
//    lines).
//    - SPRITE_OUTLINE_WHITE: White outline border (die-cut sticker border).
enum SpriteOutlineMode : uint8_t {
  SPRITE_OUTLINE_NONE = 0,
  SPRITE_OUTLINE_BLACK = 1,
  SPRITE_OUTLINE_WHITE = 2,
};

// 3. Outline Thickness / Radius
//    - SPRITE_OUTLINE_1PX: 1-pixel border (memory-efficient default)
//    - SPRITE_OUTLINE_2PX: 2-pixel border (optional for future exceptions)
enum SpriteOutlineRadius : uint8_t {
  SPRITE_OUTLINE_1PX = 1,
  SPRITE_OUTLINE_2PX = 2,
};

// --- Player Settings (Default: Solid black background, no outline) ---
static const SpriteAlphaMode PLAYER_ALPHA_MODE = SPRITE_ALPHA_OPAQUE;
static const SpriteOutlineMode PLAYER_OUTLINE_MODE = SPRITE_OUTLINE_NONE;

// --- Entities / Items Settings ---
static const SpriteAlphaMode ENTITY_ALPHA_MODE = SPRITE_ALPHA_OPAQUE;
static const SpriteOutlineMode ENTITY_OUTLINE_MODE = SPRITE_OUTLINE_BLACK;
static const SpriteOutlineRadius ENTITY_OUTLINE_RADIUS = SPRITE_OUTLINE_1PX;

// --- Whitehole Settings (Unique case: default transparent, no outline) ---
static const SpriteAlphaMode WHITEHOLE_ALPHA_MODE = SPRITE_ALPHA_TRANSPARENT;
static const SpriteOutlineMode WHITEHOLE_OUTLINE_MODE = SPRITE_OUTLINE_NONE;

#endif // CONFIG_H
