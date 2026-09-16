// =============================================================================
// player.h — Player Entity (Fat Man with Hybrid Inertia Controls)
// =============================================================================
//
// PURPOSE:
//   Defines the player's state and movement logic. The player is the fat man
//   running on the infinite plane, controlled with the D-pad + A/B buttons.
//
// CONTROL MODEL — HYBRID INERTIA:
//   This is the core "game feel" — get this right and the game is fun.
//
//   1. D-pad sets the DESIRED DIRECTION (8-dir + idle):
//      UP         → desiredDx=0,  desiredDy=-1
//      DOWN       → desiredDx=0,  desiredDy=+1
//      LEFT       → desiredDx=-1, desiredDy=0
//      RIGHT      → desiredDx=+1, desiredDy=0
//      UP+RIGHT   → desiredDx=+1, desiredDy=-1
//      (etc. for all 8 diagonals)
//      NONE       → desiredDx=0,  desiredDy=0
//
//   2. If A button is held AND a direction is set:
//      Apply THRUST in the desired direction:
//        vx += desiredDx * PLAYER_ACCEL
//        vy += desiredDy * PLAYER_ACCEL
//
//   3. Velocity BLENDING (the inertia/drift):
//      Each frame, velocity doesn't change instantly. Instead:
//        vx = vx * PLAYER_INERTIA + targetVx * (1 - PLAYER_INERTIA)
//      Where targetVx is the direction the player WANTS to go.
//      High PLAYER_INERTIA (0.85) means the fat man "drifts" — his actual
//      movement slowly catches up to the desired direction.
//      This creates the satisfying "steering a heavy object" feel.
//
//   4. FRICTION (passive deceleration):
//      Each frame, velocity is reduced:
//        if (|vx| > 0) vx -= sign(vx) * PLAYER_FRICTION
//      This means the player naturally slows down when not accelerating.
//
//   5. BRAKE (B button):
//      When B is held, friction is increased to PLAYER_BRAKE_FRICTION.
//      The player decelerates faster, useful for sharp turns (brake + new dir).
//
//   6. FOOD SLOW DEBUFF:
//      When player touches food, applyFoodSlow() sets a timer and intensity.
//      While slowed, effective max speed is reduced:
//        effectiveMaxSpeed = PLAYER_MAX_SPEED * (100 - slowIntensity) / 100
//      slowTimer counts down each frame; when 0, debuff ends.
//
//   7. SPEED CLAMPING:
//      Velocity magnitude is clamped to effectiveMaxSpeed each frame.
//      Use integer approximation: |vx| + |vy| <= maxSpeed * 1.41 (diagonal)
//      Or compute magnitude² = vx² + vy² and compare to maxSpeed² (cheaper
//      than sqrt, but watch for int32_t overflow on squared Q8.8 values).
//
// ARCHITECTURE:
//   100% PLATFORM-INDEPENDENT. No HAL, no Arduboy headers.
//   Rendering is handled by Game::renderPlayer() in game.cpp.
//
// FOR IMPLEMENTING AGENTS (M1):
//   This is the FIRST gameplay milestone. Get the update() loop right:
//   1. Read desired direction from input booleans
//   2. Apply thrust if A held
//   3. Blend velocity with inertia
//   4. Apply friction (or brake friction if B held)
//   5. Apply food slow debuff modifier
//   6. Clamp velocity to max speed
//   7. Update position: x += vx, y += vy
//   Playtest extensively! The feel should be: heavy, drifty, satisfying.
//
// =============================================================================

#ifndef PLAYER_H
#define PLAYER_H

#include "config.h"
#include "entity.h"  // For EntityType (used in applyFoodSlow)

// =============================================================================
// Player — The fat man you control
// =============================================================================
// Size: ~30 bytes. Only one instance exists (in Game).

struct Player {
    // ---- Position (world space, Q24.8 for large coordinate range) ----
    fp32_t x;   // World X position (center of player hitbox)
    fp32_t y;   // World Y position (center of player hitbox)

    // ---- Velocity (Q8.8 — values stay small: -2.0 to +2.0 typically) ----
    fp_t vx;    // Current velocity X component
    fp_t vy;    // Current velocity Y component

    // ---- Input state (set each frame from D-pad) ----
    int8_t desiredDx;  // Desired X direction: -1 (left), 0 (none), +1 (right)
    int8_t desiredDy;  // Desired Y direction: -1 (up),   0 (none), +1 (down)

    // ---- Food slow debuff ----
    uint8_t slowTimer;      // Frames remaining of slow effect (0 = not slowed)
    uint8_t slowIntensity;  // Speed reduction percentage (0-100) during slow

    // ---- Hitbox (pixels) ----
    uint8_t width;   // = PLAYER_WIDTH  (10)
    uint8_t height;  // = PLAYER_HEIGHT (10)

    // =========================================================================
    // init() — Reset player to starting state
    // =========================================================================
    // Called at game start and on retry. Sets position to starting point,
    // zeroes velocity, clears debuffs.
    //
    // Starting position: (0, 0) in world space. The camera centers on the
    // player, so the player appears at screen center initially.
    // The blackhole starts BH_START_DISTANCE pixels away.
    void init();

    // =========================================================================
    // update(up, down, left, right, accel, brake) — Per-frame movement update
    // =========================================================================
    // THE most important function in the game. This implements the hybrid
    // inertia control model described above.
    //
    // Parameters (all booleans from Input::pressed()):
    //   up    — D-pad UP is held
    //   down  — D-pad DOWN is held
    //   left  — D-pad LEFT is held
    //   right — D-pad RIGHT is held
    //   accel — A button is held (accelerate)
    //   brake — B button is held (brake)
    //
    // Algorithm (implement in this order):
    //   1. Set desiredDx/desiredDy from directional booleans:
    //      desiredDx = (right ? 1 : 0) - (left ? 1 : 0)
    //      desiredDy = (down  ? 1 : 0) - (up   ? 1 : 0)
    //
    //   2. If accel AND (desiredDx != 0 OR desiredDy != 0):
    //      // Apply thrust in desired direction
    //      targetVx = desiredDx * PLAYER_ACCEL   (use FP multiplication)
    //      targetVy = desiredDy * PLAYER_ACCEL
    //      // Blend current velocity toward target (inertia)
    //      vx = FP_MUL(vx, PLAYER_INERTIA) + FP_MUL(targetVx, FP_ONE - PLAYER_INERTIA)
    //      vy = FP_MUL(vy, PLAYER_INERTIA) + FP_MUL(targetVy, FP_ONE - PLAYER_INERTIA)
    //      // Also add direct thrust for responsiveness:
    //      vx += targetVx
    //      vy += targetVy
    //
    //   3. Apply friction:
    //      frictionAmount = brake ? PLAYER_BRAKE_FRICTION : PLAYER_FRICTION
    //      // Reduce velocity magnitude toward zero:
    //      if (vx > 0) vx = max(0, vx - frictionAmount)
    //      if (vx < 0) vx = min(0, vx + frictionAmount)
    //      // Same for vy
    //
    //   4. Calculate effective max speed:
    //      maxSpd = PLAYER_MAX_SPEED
    //      if (slowTimer > 0):
    //        maxSpd = FP_MUL(maxSpd, INT_TO_FP(100 - slowIntensity)) / 100
    //        // Or: maxSpd = maxSpd * (100 - slowIntensity) / 100
    //        slowTimer--
    //
    //   5. Clamp velocity to maxSpd:
    //      // Approximate magnitude: speed² = vx² + vy²
    //      // If speed² > maxSpd², scale down: vx = vx * maxSpd / speed
    //      // For simplicity, can clamp each axis independently:
    //      vx = clamp(vx, -maxSpd, +maxSpd)
    //      vy = clamp(vy, -maxSpd, +maxSpd)
    //
    //   6. Update position:
    //      x += vx   (fp32_t += fp_t is safe, widening conversion)
    //      y += vy
    //
    void update(bool up, bool down, bool left, bool right, bool accel, bool brake);

    // =========================================================================
    // applyFoodSlow(foodType) — Apply a food slow debuff
    // =========================================================================
    // Called when player collides with a food entity.
    // Sets slowTimer and slowIntensity based on the food type.
    // If already slowed, the NEW slow REPLACES the old one (doesn't stack).
    //
    // Parameters:
    //   foodType — The EntityType of the food that was touched.
    //
    // Behavior by type (constants from config.h):
    //   ENTITY_FOOD_PIZZA:  timer=30,  intensity=15  (light, brief)
    //   ENTITY_FOOD_BURGER: timer=60,  intensity=30  (medium)
    //   ENTITY_FOOD_DONUT:  timer=90,  intensity=50  (heavy, long)
    //   Other types: do nothing (shouldn't be called with non-food)
    void applyFoodSlow(EntityType foodType);

    // =========================================================================
    // isSlowed() — Check if player is currently under food slow debuff
    // =========================================================================
    // Returns: true if slowTimer > 0
    bool isSlowed() const;
};

#endif // PLAYER_H
