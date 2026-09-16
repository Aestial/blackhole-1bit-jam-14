// =============================================================================
// player.cpp — Player Movement & Debuff Implementation
// =============================================================================
//
// MILESTONE: M1 (Player Movement with Hybrid Inertia Physics)
//
// FOR IMPLEMENTING AGENTS:
//   This is the FIRST gameplay milestone. The update() function is the heart
//   of the game feel. Follow the algorithm documented in player.h precisely.
//   Playtest at 60 FPS on ProjectABE emulator to get the feel right.
//
//   Tips:
//   - Start with just position += velocity, then add inertia blending
//   - Diagonal movement should NOT be faster (normalize or clamp)
//   - The food slow effect should be noticeable but not unfun
//   - Print vx/vy to screen during debug to see values
//
// =============================================================================

#include "player.h"

// -----------------------------------------------------------------------------
// Player::init()
// -----------------------------------------------------------------------------
void Player::init() {
    x = 0;
    y = 0;
    vx = 0;
    vy = 0;
    desiredDx = 0;
    desiredDy = 0;
    slowTimer = 0;
    slowIntensity = 0;
    width = PLAYER_WIDTH;
    height = PLAYER_HEIGHT;
}

// -----------------------------------------------------------------------------
// Player::update()
// TODO(M1): Implement hybrid inertia movement — see player.h for full algorithm
// -----------------------------------------------------------------------------
void Player::update(bool up, bool down, bool left, bool right, bool accel, bool brake) {
    // TODO(M1): Implement the 6-step algorithm from player.h:
    //
    // Step 1: Set desired direction from input booleans
    //   desiredDx = (right ? 1 : 0) - (left ? 1 : 0);
    //   desiredDy = (down  ? 1 : 0) - (up   ? 1 : 0);
    //
    // Step 2: If accelerating with a direction, apply thrust + inertia blend
    //   - Calculate target velocity from desired direction × PLAYER_ACCEL
    //   - Blend current velocity toward target using PLAYER_INERTIA
    //   - Add direct thrust for responsiveness
    //
    // Step 3: Apply friction (PLAYER_FRICTION or PLAYER_BRAKE_FRICTION if braking)
    //   - Reduce vx/vy toward zero by friction amount
    //
    // Step 4: Calculate effective max speed (apply food slow if active)
    //   - If slowTimer > 0, reduce max speed by slowIntensity percent
    //   - Decrement slowTimer
    //
    // Step 5: Clamp velocity to effective max speed
    //   - Prevent player from exceeding max speed on either axis
    //
    // Step 6: Update position
    //   - x += vx, y += vy

    // STUB: Minimal movement for testing (replace with full algorithm in M1)
    desiredDx = (right ? 1 : 0) - (left ? 1 : 0);
    desiredDy = (down  ? 1 : 0) - (up   ? 1 : 0);

    if (accel) {
        vx += desiredDx * PLAYER_ACCEL;
        vy += desiredDy * PLAYER_ACCEL;
    }

    // Basic friction
    fp_t friction = brake ? PLAYER_BRAKE_FRICTION : PLAYER_FRICTION;
    if (vx > 0) { vx -= friction; if (vx < 0) vx = 0; }
    if (vx < 0) { vx += friction; if (vx > 0) vx = 0; }
    if (vy > 0) { vy -= friction; if (vy < 0) vy = 0; }
    if (vy < 0) { vy += friction; if (vy > 0) vy = 0; }

    // Basic speed clamp (per-axis, no inertia blending yet)
    fp_t maxSpd = PLAYER_MAX_SPEED;
    if (slowTimer > 0) {
        maxSpd = (fp_t)((int32_t)maxSpd * (100 - slowIntensity) / 100);
        slowTimer--;
    }
    if (vx >  maxSpd) vx =  maxSpd;
    if (vx < -maxSpd) vx = -maxSpd;
    if (vy >  maxSpd) vy =  maxSpd;
    if (vy < -maxSpd) vy = -maxSpd;

    // Update position
    x += vx;
    y += vy;
}

// -----------------------------------------------------------------------------
// Player::applyFoodSlow()
// -----------------------------------------------------------------------------
void Player::applyFoodSlow(EntityType foodType) {
    switch (foodType) {
        case ENTITY_FOOD_PIZZA:
            slowTimer = FOOD_PIZZA_SLOW_DURATION;
            slowIntensity = FOOD_PIZZA_SLOW_INTENSITY;
            break;
        case ENTITY_FOOD_BURGER:
            slowTimer = FOOD_BURGER_SLOW_DURATION;
            slowIntensity = FOOD_BURGER_SLOW_INTENSITY;
            break;
        case ENTITY_FOOD_DONUT:
            slowTimer = FOOD_DONUT_SLOW_DURATION;
            slowIntensity = FOOD_DONUT_SLOW_INTENSITY;
            break;
        default:
            // Not a food type — do nothing
            break;
    }
}

// -----------------------------------------------------------------------------
// Player::isSlowed()
// -----------------------------------------------------------------------------
bool Player::isSlowed() const {
    return slowTimer > 0;
}
