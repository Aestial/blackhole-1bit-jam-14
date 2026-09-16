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
    slowTimerAccum = 0;
    width = PLAYER_WIDTH;
    height = PLAYER_HEIGHT;
}

// -----------------------------------------------------------------------------
// Player::update()
// -----------------------------------------------------------------------------
void Player::update(bool up, bool down, bool left, bool right, bool accel, bool brake, fp_t dt) {
    desiredDx = (right ? 1 : 0) - (left ? 1 : 0);
    desiredDy = (down  ? 1 : 0) - (up   ? 1 : 0);

    if (accel) {
        vx += FP_MUL(desiredDx * PLAYER_ACCEL, dt);
        vy += FP_MUL(desiredDy * PLAYER_ACCEL, dt);
    }

    // Friction scaled by dt
    fp_t friction = brake ? PLAYER_BRAKE_FRICTION : PLAYER_FRICTION;
    fp_t fStep = FP_MUL(friction, dt);
    if (vx > 0) { vx -= fStep; if (vx < 0) vx = 0; }
    if (vx < 0) { vx += fStep; if (vx > 0) vx = 0; }
    if (vy > 0) { vy -= fStep; if (vy < 0) vy = 0; }
    if (vy < 0) { vy += fStep; if (vy > 0) vy = 0; }

    // Speed clamp (apply food slow if active)
    fp_t maxSpd = PLAYER_MAX_SPEED;
    if (slowTimer > 0) {
        maxSpd = (fp_t)((int32_t)maxSpd * (100 - slowIntensity) / 100);
        slowTimerAccum += dt;
        while (slowTimerAccum >= FP_DT_ONE && slowTimer > 0) {
            slowTimerAccum -= FP_DT_ONE;
            slowTimer--;
        }
    } else {
        slowTimerAccum = 0;
    }
    if (vx >  maxSpd) vx =  maxSpd;
    if (vx < -maxSpd) vx = -maxSpd;
    if (vy >  maxSpd) vy =  maxSpd;
    if (vy < -maxSpd) vy = -maxSpd;

    // Update position scaled by dt
    x += FP32_MUL(vx, dt);
    y += FP32_MUL(vy, dt);
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
        case ENTITY_FOOD_ICECREAM:
            slowTimer = FOOD_ICECREAM_SLOW_DURATION;
            slowIntensity = FOOD_ICECREAM_SLOW_INTENSITY;
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
