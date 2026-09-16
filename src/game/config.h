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
// Define the target platform here. When porting to a new platform (e.g., Raylib),
// add a new #define and update src/hal/hal_types.h accordingly.
// Only ONE platform should be defined at a time.

#ifndef PLATFORM_ARDUBOY
#define PLATFORM_ARDUBOY
#endif

// =============================================================================
// SCREEN DIMENSIONS
// =============================================================================
// Arduboy: 128x64 pixels, 1-bit (monochrome: black=0, white=1)
// These are used for screen-space calculations, HUD placement, and spawn bounds.

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
#define FP_ONE   (1 << FP_SHIFT)       // = 256, represents 1.0
#define FP_HALF  (1 << (FP_SHIFT - 1)) // = 128, represents 0.5

// Type aliases for clarity
typedef int16_t fp_t;    // Q8.8 fixed-point (range: -128 to ~+128)
typedef int32_t fp32_t;  // Q16.8 or Q24.8 for world coordinates (large range)

// Convert integer to fixed-point: e.g., INT_TO_FP(3) = 768
#define INT_TO_FP(x) ((fp_t)((int16_t)(x) << FP_SHIFT))

// Convert fixed-point to integer (truncates fractional part): e.g., FP_TO_INT(768) = 3
#define FP_TO_INT(x) ((int16_t)((x) >> FP_SHIFT))

// Convert float to fixed-point at compile time ONLY (for initializing constants):
// e.g., FLOAT_TO_FP(1.5) = 384. NEVER use at runtime — no FPU!
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
// Player position, camera, and blackhole use fp32_t (int32_t with 8-bit fraction).
// Same shift factor — all FP macros work, but use FP32 variants for 32-bit ops.

#define INT_TO_FP32(x)  ((fp32_t)((int32_t)(x) << FP_SHIFT))
#define FP32_TO_INT(x)  ((int32_t)((x) >> FP_SHIFT))
#define FP32_MUL(a, b)  ((fp32_t)(((int64_t)(a) * (int64_t)(b)) >> FP_SHIFT))

// =============================================================================
// FRAME RATE
// =============================================================================

static const uint8_t TARGET_FPS = 60;

// Delta-time normalization: 1.0 in Q8.8 represents exactly one 60 FPS frame (16.67ms)
#define FP_DT_ONE FP_ONE

// =============================================================================
// ANIMATION TIMING CONSTANTS
// =============================================================================
// Divisors to convert frame/spin ticks into sprite animation frames.
// Higher divisor = slower, smoother, more deliberate animation.
static const uint8_t WHITEHOLE_ANIM_DIVISOR = 12; // 60 FPS / 12 = 5 FPS sprite animation (~0.8s per revolution)
static const uint8_t PLAYER_BLINK_DIVISOR   = 8;  // 60 FPS / 8 = 7.5 Hz debuff blink cadence

// =============================================================================
// PLAYER PHYSICS CONSTANTS (HYBRID INERTIA)
// =============================================================================
// The player uses a HYBRID INERTIA control model:
//   - D-pad sets the "desired direction" (8-dir + idle, normalized diagonally)
//   - Velocity BLENDS toward the desired direction over time (inertia)
//   - A button applies thrust (accelerates in desired direction)
//   - B button applies extra friction (brakes)
//   - The fat man feels heavy, drifts when turning, and takes time to stop
//
// Tuning parameters:
//   PLAYER_ACCEL: thrust per frame (responsive takeoff)
//   PLAYER_FRICTION: passive drag per frame (smooth glide)
//   PLAYER_BRAKE_FRICTION: drag when B held (responsive braking/drifting)
//   PLAYER_MAX_SPEED: velocity magnitude cap (60 px/sec across 128px screen)
//   PLAYER_INERTIA: velocity blend factor in Q8.8 (0.85 = heavy drift)
//   DIAGONAL_FACTOR: 1/sqrt(2) in Q8.8 (181/256 = 0.7071) for equal 8-dir speed

static const fp_t PLAYER_ACCEL          = FLOAT_TO_FP(0.09);  // Thrust per frame
static const fp_t PLAYER_FRICTION       = FLOAT_TO_FP(0.012); // Passive drag per frame
static const fp_t PLAYER_BRAKE_FRICTION = FLOAT_TO_FP(0.06);  // Drag when B held
static const fp_t PLAYER_MAX_SPEED      = FLOAT_TO_FP(1.05);  // Max velocity magnitude
static const fp_t PLAYER_INERTIA        = FLOAT_TO_FP(0.85);  // Velocity blend factor (0-1)
static const fp_t DIAGONAL_FACTOR       = 181;                // 1/sqrt(2) in Q8.8 (~0.7071)

// Player hitbox size in pixels (used for collision detection)
static const uint8_t PLAYER_WIDTH  = 10;
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

// Entity sizes (pixels) — used for both rendering and collision hitboxes
static const uint8_t FOOD_PIZZA_SIZE      = 6;   // Rendered as triangle
static const uint8_t FOOD_BURGER_SIZE     = 6;   // Rendered as filled square
static const uint8_t FOOD_DONUT_SIZE      = 4;   // Rendered as small circle
static const uint8_t FOOD_ICECREAM_SIZE   = 6;   // Rendered as ice cream cone
static const uint8_t COLLECTIBLE_SIZE     = 4;   // Rendered as diamond / gem

// =============================================================================
// FOOD SLOWDOWN CONSTANTS
// =============================================================================
// When the fat man touches food, he's temporarily slowed.
// Duration is in frames (at 60 FPS): 30 frames = 0.5 seconds.
// Intensity is a percentage of speed REDUCTION (0-100):
//   0   = no slow
//   50  = half speed
//   100 = full stop (don't use — feels unfair)
//
// Design intent:
//   Pizza    = light snack, barely slows you
//   Burger   = heavier meal, noticeable
//   Donut    = irresistible, significant slow
//   IceCream = brain freeze, heavy slow

static const uint8_t FOOD_PIZZA_SLOW_DURATION    = 30;   // ~0.5 sec
static const uint8_t FOOD_PIZZA_SLOW_INTENSITY    = 15;   // -15% speed

static const uint8_t FOOD_BURGER_SLOW_DURATION   = 60;   // ~1.0 sec
static const uint8_t FOOD_BURGER_SLOW_INTENSITY   = 30;   // -30% speed

static const uint8_t FOOD_DONUT_SLOW_DURATION    = 90;   // ~1.5 sec
static const uint8_t FOOD_DONUT_SLOW_INTENSITY    = 50;   // -50% speed

static const uint8_t FOOD_ICECREAM_SLOW_DURATION = 120;  // ~2.0 sec (Brain freeze!)
static const uint8_t FOOD_ICECREAM_SLOW_INTENSITY = 60;  // -60% speed

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

static const fp_t BH_BASE_SPEED          = FLOAT_TO_FP(0.20);  // Initial chase speed (~12 px/sec)
static const fp_t BH_SPEED_INCREMENT     = 1;                  // Speed increase step in Q8.8 (1/256)
static const uint8_t BH_SPEED_RAMP_INTERVAL  = 120;             // Frames between speed increases (~2 sec)
static const fp_t BH_MASS                = FLOAT_TO_FP(1.0);   // Gravity pull strength (fixed)
static const fp_t BH_BASE_CHARGE         = INT_TO_FP(30);      // Initial attraction radius (pixels)
static const fp_t BH_CHARGE_INCREMENT    = 1;                  // Charge increase step in Q8.8
static const uint8_t BH_CHARGE_RAMP_INTERVAL = 120;            // Frames between charge increases (~2 sec)

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

static const uint8_t SPAWN_INTERVAL_START   = 90;   // Frames between spawns (initial)
static const uint8_t SPAWN_MIN_INTERVAL     = 20;   // Minimum spawn interval
static const uint8_t SPAWN_RAMP_INTERVAL    = 120;  // Frames between spawn rate increases
static const uint8_t SPAWN_RADIUS           = 80;   // Max distance from player to spawn
static const uint8_t SPAWN_MIN_DISTANCE     = 30;   // Min distance from player to spawn
static const uint8_t DESPAWN_DISTANCE       = 120;  // Distance from camera edge to despawn

// =============================================================================
// SCORING
// =============================================================================
// Points for collectibles and passive survival time.

static const uint8_t SCORE_PER_COLLECTIBLE = 10;  // Points per collected item
static const uint8_t SCORE_TIME_INTERVAL   = 60;  // Frames between passive score ticks
static const uint8_t SCORE_PER_TICK        = 1;   // Points per time tick

// =============================================================================
// EEPROM
// =============================================================================
// Address in EEPROM to store the high score (uint16_t = 2 bytes).
// Arduboy reserves EEPROM 0-15 for system use. Start at 16.

static const uint16_t EEPROM_HIGH_SCORE_ADDR = 16;

// =============================================================================
// PERSPECTIVE GROUND PLANE
// =============================================================================
// Parameters for the pseudo-3D perspective ground grid:
// - Horizon line Y: separates celestial sky void from the ground plane
// - Ground depth Z at player: depth corresponding to screen center Y = 32
// - Spacing of perspective rays and depth lines
static const int16_t HORIZON_Y            = 14;  // Horizon line Y coordinate
static const int16_t GROUND_DEPTH_PLAYER  = 72;  // Depth Z at player (screen Y = 32)
static const int16_t BASE_SPACING_X       = 24;  // Ray spacing at screen bottom
static const int16_t TOP_SPACING_X        = 6;   // Ray spacing at horizon line
static const int16_t Z_PERIOD             = 24;  // Depth line period

static const uint8_t GRID_SPACING         = 16;  // Pixels between grid lines

// =============================================================================
// COLORS (1-bit: 0=BLACK, 1=WHITE)
// =============================================================================
// Named constants for clarity in rendering code.

static const uint8_t COLOR_BLACK = 0;
static const uint8_t COLOR_WHITE = 1;

#endif // CONFIG_H
