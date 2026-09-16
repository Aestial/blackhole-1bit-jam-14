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
#include "physics.h"

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
    // 1. Calculate desired direction from D-pad inputs
    desiredDx = (right ? 1 : 0) - (left ? 1 : 0);
    desiredDy = (down  ? 1 : 0) - (up   ? 1 : 0);

    // 2. Determine effective max speed (adjusted for food slow debuff)
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

    // 3. Directional thrust & inertia blending
    if (accel && (desiredDx != 0 || desiredDy != 0)) {
        // Diagonal normalization in Q8.8 (DIAGONAL_FACTOR = 181 ~= 0.7071)
        fp_t dirX, dirY;
        if (desiredDx != 0 && desiredDy != 0) {
            dirX = desiredDx * DIAGONAL_FACTOR;
            dirY = desiredDy * DIAGONAL_FACTOR;
        } else {
            dirX = INT_TO_FP(desiredDx);
            dirY = INT_TO_FP(desiredDy);
        }

        // Target velocity vector based on desired heading and max speed
        fp_t targetVx = FP_MUL(dirX, maxSpd);
        fp_t targetVy = FP_MUL(dirY, maxSpd);

        // Inertia blending: smooth velocity vector steering towards target direction
        fp_t blend = PLAYER_INERTIA;
        vx = FP_MUL(vx, blend) + FP_MUL(targetVx, FP_ONE - blend);
        vy = FP_MUL(vy, blend) + FP_MUL(targetVy, FP_ONE - blend);

        // Direct acceleration step scaled by delta-time
        fp_t thrustStep = FP_MUL(PLAYER_ACCEL, dt);
        vx += FP_MUL(dirX, thrustStep);
        vy += FP_MUL(dirY, thrustStep);
    }

    // 4. Deceleration (Active braking or passive friction)
    if (brake) {
        // Holding B applies strong braking friction
        fp_t fStep = FP_MUL(PLAYER_BRAKE_FRICTION, dt);
        if (vx > 0) { vx -= fStep; if (vx < 0) vx = 0; }
        if (vx < 0) { vx += fStep; if (vx > 0) vx = 0; }
        if (vy > 0) { vy -= fStep; if (vy < 0) vy = 0; }
        if (vy < 0) { vy += fStep; if (vy > 0) vy = 0; }
    } else if (!accel) {
        // When not accelerating, apply passive drag for smooth drift/gliding
        fp_t fStep = FP_MUL(PLAYER_FRICTION, dt);
        if (vx > 0) { vx -= fStep; if (vx < 0) vx = 0; }
        if (vx < 0) { vx += fStep; if (vx > 0) vx = 0; }
        if (vy > 0) { vy -= fStep; if (vy < 0) vy = 0; }
        if (vy < 0) { vy += fStep; if (vy > 0) vy = 0; }
    }

    // 5. Circular velocity magnitude clamping (guarantees equal max speed in all directions)
    int32_t speedSq = (int32_t)vx * vx + (int32_t)vy * vy;
    int32_t maxSpdSq = (int32_t)maxSpd * maxSpd;
    if (speedSq > maxSpdSq) {
        fp_t currentSpeed = (fp_t)approxDistance(0, 0, vx, vy);
        if (currentSpeed > 0) {
            vx = (fp_t)(((int32_t)vx * maxSpd) / currentSpeed);
            vy = (fp_t)(((int32_t)vy * maxSpd) / currentSpeed);
        }
    }

    // 6. Integrate position scaled by delta-time
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
