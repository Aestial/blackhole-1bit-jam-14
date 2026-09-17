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
    facingDir = 0; // PLAYER_DIR_DOWN
    walkFrame = 0;
    animTimer = 0;
    slowTimer = 0;
    slowIntensity = 0;
    slowTimerAccum = 0;
    boostTimer = 0;
    boostTimerAccum = 0;
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

    // Update 8-directional facing orientation from D-pad input
    if (desiredDx != 0 || desiredDy != 0) {
        if (desiredDx == 0 && desiredDy > 0)       facingDir = 0; // DOWN
        else if (desiredDx > 0 && desiredDy > 0)  facingDir = 1; // DOWN_RIGHT
        else if (desiredDx > 0 && desiredDy == 0)  facingDir = 2; // RIGHT
        else if (desiredDx > 0 && desiredDy < 0)  facingDir = 3; // UP_RIGHT
        else if (desiredDx == 0 && desiredDy < 0)  facingDir = 4; // UP
        else if (desiredDx < 0 && desiredDy < 0)  facingDir = 5; // UP_LEFT
        else if (desiredDx < 0 && desiredDy == 0)  facingDir = 6; // LEFT
        else if (desiredDx < 0 && desiredDy > 0)  facingDir = 7; // DOWN_LEFT
    }

    // 2. Determine effective max speed & acceleration (adjusted for boost or food slow)
    fp_t maxSpd = PLAYER_MAX_SPEED;
    fp_t accelVal = PLAYER_ACCEL;

    if (boostTimer > 0) {
        maxSpd = FP_MUL(maxSpd, POWERUP_COFFEE_SPEED_BOOST);
        accelVal = FP_MUL(accelVal, POWERUP_COFFEE_ACCEL_BOOST);
        boostTimerAccum += dt;
        while (boostTimerAccum >= FP_DT_ONE && boostTimer > 0) {
            boostTimerAccum -= FP_DT_ONE;
            boostTimer--;
        }
    } else {
        boostTimerAccum = 0;
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
    }

    // 3. Directional movement & agile human steering
    bool hasDir = (desiredDx != 0 || desiredDy != 0);
    fp_t currentSpeed = (fp_t)approxDistance(0, 0, vx, vy);

    if (hasDir) {
        // Diagonal normalization in Q8.8 (DIAGONAL_FACTOR = 181 ~= 0.7071)
        fp_t dirX, dirY;
        if (desiredDx != 0 && desiredDy != 0) {
            dirX = desiredDx * DIAGONAL_FACTOR;
            dirY = desiredDy * DIAGONAL_FACTOR;
        } else {
            dirX = INT_TO_FP(desiredDx);
            dirY = INT_TO_FP(desiredDy);
        }

        if (accel) {
            // Gas input (walk): small grounded impulse
            // If starting from a standstill, apply an initial stride impulse
            if (currentSpeed < PLAYER_STEP_IMPULSE) {
                currentSpeed = PLAYER_STEP_IMPULSE;
            } else {
                currentSpeed += FP_MUL(accelVal, dt);
                if (currentSpeed > maxSpd) currentSpeed = maxSpd;
            }
        }

        // Steering: humans turn and redirect their velocity quickly
        // Small inertia feedback (0.25) provides body weight without wide car-like drifting
        fp_t targetVx = FP_MUL(dirX, currentSpeed);
        fp_t targetVy = FP_MUL(dirY, currentSpeed);

        fp_t blend = PLAYER_INERTIA;
        vx = FP_MUL(vx, blend) + FP_MUL(targetVx, FP_ONE - blend);
        vy = FP_MUL(vy, blend) + FP_MUL(targetVy, FP_ONE - blend);
    }

    // 4. Deceleration (Active braking or natural foot friction)
    if (brake) {
        // Holding B applies firm foot-plant braking (stops in ~8 frames)
        fp_t fStep = FP_MUL(PLAYER_BRAKE_FRICTION, dt);
        if (vx > 0) { vx -= fStep; if (vx < 0) vx = 0; }
        if (vx < 0) { vx += fStep; if (vx > 0) vx = 0; }
        if (vy > 0) { vy -= fStep; if (vy < 0) vy = 0; }
        if (vy < 0) { vy += fStep; if (vy > 0) vy = 0; }
    } else if (!accel) {
        // When not walking, natural foot drag brings fat man to a stop in ~20 frames
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
        fp_t finalSpeed = (fp_t)approxDistance(0, 0, vx, vy);
        if (finalSpeed > 0) {
            vx = (fp_t)(((int32_t)vx * maxSpd) / finalSpeed);
            vy = (fp_t)(((int32_t)vy * maxSpd) / finalSpeed);
        }
    }

    // 6. Integrate position scaled by delta-time
    x += FP32_MUL(vx, dt);
    y += FP32_MUL(vy, dt);

    // 7. Update walk cycle animation based on movement
    fp_t currentSpeedAfter = (fp_t)approxDistance(0, 0, vx, vy);
    if (currentSpeedAfter > FLOAT_TO_FP(0.08)) {
        animTimer++;
        if (animTimer >= PLAYER_WALK_ANIM_DIVISOR) {
            animTimer = 0;
            walkFrame = (walkFrame == 0) ? 1 : 0;
        }
    } else {
        walkFrame = 0;
        animTimer = 0;
    }
}

// -----------------------------------------------------------------------------
// Player::applyFoodSlow()
// -----------------------------------------------------------------------------
void Player::applyFoodSlow(EntityType foodType) {
    switch (foodType) {
        case ENTITY_FOOD_APPLE:
            slowTimer = FOOD_APPLE_SLOW_DURATION;
            slowIntensity = FOOD_APPLE_SLOW_INTENSITY;
            break;
        case ENTITY_FOOD_PIZZA:
            slowTimer = FOOD_PIZZA_SLOW_DURATION;
            slowIntensity = FOOD_PIZZA_SLOW_INTENSITY;
            break;
        case ENTITY_FOOD_TACO:
            slowTimer = FOOD_TACO_SLOW_DURATION;
            slowIntensity = FOOD_TACO_SLOW_INTENSITY;
            break;
        case ENTITY_FOOD_BURGER:
            slowTimer = FOOD_BURGER_SLOW_DURATION;
            slowIntensity = FOOD_BURGER_SLOW_INTENSITY;
            break;
        case ENTITY_FOOD_FRIES:
            slowTimer = FOOD_FRIES_SLOW_DURATION;
            slowIntensity = FOOD_FRIES_SLOW_INTENSITY;
            break;
        case ENTITY_FOOD_CAKE:
            slowTimer = FOOD_CAKE_SLOW_DURATION;
            slowIntensity = FOOD_CAKE_SLOW_INTENSITY;
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
// Player::applyCoffeeBoost()
// -----------------------------------------------------------------------------
void Player::applyCoffeeBoost() {
    boostTimer = POWERUP_COFFEE_DURATION;
    boostTimerAccum = 0;
    // Coffee cleanses any active food slow debuff!
    slowTimer = 0;
    slowTimerAccum = 0;
}

// -----------------------------------------------------------------------------
// Player::isSlowed()
// -----------------------------------------------------------------------------
bool Player::isSlowed() const {
    return slowTimer > 0;
}

// -----------------------------------------------------------------------------
// Player::isBoosted()
// -----------------------------------------------------------------------------
bool Player::isBoosted() const {
    return boostTimer > 0;
}
