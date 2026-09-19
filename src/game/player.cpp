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

    // Stamina system (GTA:SA-style)
    stamina = STAMINA_START;
    staminaDrainAccum = 0;
    staminaRegenAccum = 0;

    // Sprint tap detection state machine
    moveTier = MOVE_IDLE;
    sprintTapTimer = 255;     // Expired — no recent tap
    sprintTapInterval = 255;  // No measured interval yet
    sprintTapCount = 0;
    aPrevPressed = false;
    aHoldTimer = 0;
}

// -----------------------------------------------------------------------------
// Player::update()
// -----------------------------------------------------------------------------
void Player::update(bool up, bool down, bool left, bool right, bool accel, bool justPressedA, fp_t dt) {
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

    // 2. Sprint tap detection state machine
    // a) Detect A press edge (justPressedA)
    if (justPressedA) {
        sprintTapInterval = sprintTapTimer;
        if (sprintTapInterval < SPRINT_TAP_WINDOW) {
            if (sprintTapCount < 255) sprintTapCount++;
        } else {
            sprintTapCount = 1;
        }
        sprintTapTimer = 0;
        aHoldTimer = 0;
    }

    // b) While A is held, measure continuous hold duration
    if (accel) {
        if (aHoldTimer < 255) aHoldTimer++;
    } else {
        aHoldTimer = 0;
    }

    // c) Increment sprintTapTimer (cap at 255 to prevent overflow)
    if (sprintTapTimer < 255) {
        sprintTapTimer++;
    }

    // d) Record previous pressed state
    aPrevPressed = accel;

    // 3. Determine movement tier
    bool hasDir = (desiredDx != 0 || desiredDy != 0);

    if (!hasDir) {
        moveTier = MOVE_IDLE;
    } else if (!accel) {
        moveTier = MOVE_WALK;
    } else if (sprintTapCount >= SPRINT_TAP_COUNT_NEEDED &&
               aHoldTimer <= SPRINT_SUSTAIN_WINDOW &&
               stamina >= STAMINA_MIN_TO_SPRINT) {
        moveTier = MOVE_SPRINT;
    } else if (stamina >= STAMINA_MIN_TO_RUN) {
        moveTier = MOVE_RUN;
    } else {
        moveTier = MOVE_WALK; // Low stamina fallback
    }

    // 4. Compute effective base speed & acceleration from tier
    fp_t maxSpd = 0;
    fp_t accelVal = 0;

    switch (moveTier) {
        case MOVE_IDLE:
            maxSpd = 0;
            accelVal = 0;
            break;
        case MOVE_WALK:
            maxSpd = PLAYER_WALK_SPEED;
            accelVal = PLAYER_WALK_ACCEL;
            break;
        case MOVE_RUN:
            maxSpd = PLAYER_RUN_SPEED;
            accelVal = PLAYER_RUN_ACCEL;
            break;
        case MOVE_SPRINT: {
            fp_t speedRange = PLAYER_SPRINT_SPEED - PLAYER_RUN_SPEED;
            uint8_t clampedInterval = (sprintTapInterval < SPRINT_TAP_WINDOW) ? sprintTapInterval : SPRINT_TAP_WINDOW;
            fp_t factor = (fp_t)(((int32_t)(SPRINT_TAP_WINDOW - clampedInterval) << FP_SHIFT) / SPRINT_TAP_WINDOW);
            maxSpd = PLAYER_RUN_SPEED + FP_MUL(speedRange, factor);
            accelVal = PLAYER_SPRINT_ACCEL;
            break;
        }
    }

    // 5. Apply boost/slow modifiers
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

    // 6. Stamina drain and regeneration
    if (moveTier == MOVE_SPRINT) {
        staminaDrainAccum++;
        if (staminaDrainAccum >= STAMINA_SPRINT_DRAIN_INTERVAL) {
            drainStamina(STAMINA_SPRINT_DRAIN);
            staminaDrainAccum = 0;
        }
        staminaRegenAccum = 0; // No regen while sprinting
    } else if (moveTier == MOVE_RUN) {
        staminaDrainAccum++;
        if (staminaDrainAccum >= STAMINA_RUN_DRAIN_INTERVAL) {
            drainStamina(STAMINA_RUN_DRAIN);
            staminaDrainAccum = 0;
        }
        staminaRegenAccum = 0; // No regen while running
    } else {
        // IDLE or WALK — passive regen
        staminaDrainAccum = 0;
        staminaRegenAccum++;
        if (staminaRegenAccum >= STAMINA_REGEN_INTERVAL) {
            uint8_t regenAmt = (moveTier == MOVE_IDLE) ? STAMINA_REGEN_IDLE : STAMINA_REGEN_WALK;
            recoverStamina(regenAmt);
            staminaRegenAccum = 0;
        }
    }

    // Force tier downgrade if stamina depleted mid-action
    if (stamina == 0 && moveTier == MOVE_SPRINT) {
        moveTier = MOVE_RUN;
    }
    if (stamina < STAMINA_MIN_TO_RUN && moveTier == MOVE_RUN) {
        moveTier = MOVE_WALK;
    }

    // 7. Directional movement & agile human steering
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

        // Apply impulse/accel toward tier maxSpd
        if (currentSpeed < PLAYER_STEP_IMPULSE) {
            currentSpeed = PLAYER_STEP_IMPULSE;
        } else {
            currentSpeed += FP_MUL(accelVal, dt);
            if (currentSpeed > maxSpd) currentSpeed = maxSpd;
        }

        fp_t targetVx = FP_MUL(dirX, currentSpeed);
        fp_t targetVy = FP_MUL(dirY, currentSpeed);

        fp_t blend = PLAYER_INERTIA;
        vx = FP_MUL(vx, blend) + FP_MUL(targetVx, FP_ONE - blend);
        vy = FP_MUL(vy, blend) + FP_MUL(targetVy, FP_ONE - blend);
    }

    // 8. Deceleration (Natural foot friction when no directional input)
    if (!hasDir) {
        fp_t fStep = FP_MUL(PLAYER_FRICTION, dt);
        if (vx > 0) { vx -= fStep; if (vx < 0) vx = 0; }
        if (vx < 0) { vx += fStep; if (vx > 0) vx = 0; }
        if (vy > 0) { vy -= fStep; if (vy < 0) vy = 0; }
        if (vy < 0) { vy += fStep; if (vy > 0) vy = 0; }
    }

    // 9. Circular velocity magnitude clamping
    if (hasDir && maxSpd > 0) {
        int32_t speedSq = (int32_t)vx * vx + (int32_t)vy * vy;
        int32_t maxSpdSq = (int32_t)maxSpd * maxSpd;
        if (speedSq > maxSpdSq) {
            fp_t finalSpeed = (fp_t)approxDistance(0, 0, vx, vy);
            if (finalSpeed > 0) {
                vx = (fp_t)(((int32_t)vx * maxSpd) / finalSpeed);
                vy = (fp_t)(((int32_t)vy * maxSpd) / finalSpeed);
            }
        }
    }

    // 10. Integrate position scaled by delta-time
    x += FP32_MUL(vx, dt);
    y += FP32_MUL(vy, dt);

    // 11. Update walk cycle animation based on movement
    fp_t currentSpeedAfter = (fp_t)approxDistance(0, 0, vx, vy);
    if (currentSpeedAfter > FLOAT_TO_FP(0.08)) {
        uint8_t divisor = PLAYER_WALK_ANIM_DIVISOR;
        if (moveTier == MOVE_WALK) {
            divisor = PLAYER_WALK_ANIM_DIVISOR * 2;
        } else if (moveTier == MOVE_SPRINT) {
            divisor = (PLAYER_WALK_ANIM_DIVISOR > 4) ? (PLAYER_WALK_ANIM_DIVISOR / 2) : 2;
        }
        animTimer++;
        if (animTimer >= divisor) {
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
            drainStamina(STAMINA_FOOD_APPLE_DRAIN);
            break;
        case ENTITY_FOOD_PIZZA:
            slowTimer = FOOD_PIZZA_SLOW_DURATION;
            slowIntensity = FOOD_PIZZA_SLOW_INTENSITY;
            drainStamina(STAMINA_FOOD_PIZZA_DRAIN);
            break;
        case ENTITY_FOOD_TACO:
            slowTimer = FOOD_TACO_SLOW_DURATION;
            slowIntensity = FOOD_TACO_SLOW_INTENSITY;
            drainStamina(STAMINA_FOOD_TACO_DRAIN);
            break;
        case ENTITY_FOOD_BURGER:
            slowTimer = FOOD_BURGER_SLOW_DURATION;
            slowIntensity = FOOD_BURGER_SLOW_INTENSITY;
            drainStamina(STAMINA_FOOD_BURGER_DRAIN);
            break;
        case ENTITY_FOOD_FRIES:
            slowTimer = FOOD_FRIES_SLOW_DURATION;
            slowIntensity = FOOD_FRIES_SLOW_INTENSITY;
            drainStamina(STAMINA_FOOD_FRIES_DRAIN);
            break;
        case ENTITY_FOOD_CAKE:
            slowTimer = FOOD_CAKE_SLOW_DURATION;
            slowIntensity = FOOD_CAKE_SLOW_INTENSITY;
            drainStamina(STAMINA_FOOD_CAKE_DRAIN);
            break;
        case ENTITY_FOOD_DONUT:
            slowTimer = FOOD_DONUT_SLOW_DURATION;
            slowIntensity = FOOD_DONUT_SLOW_INTENSITY;
            drainStamina(STAMINA_FOOD_DONUT_DRAIN);
            break;
        case ENTITY_FOOD_ICECREAM:
            slowTimer = FOOD_ICECREAM_SLOW_DURATION;
            slowIntensity = FOOD_ICECREAM_SLOW_INTENSITY;
            drainStamina(STAMINA_FOOD_ICECREAM_DRAIN);
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
    recoverStamina(STAMINA_COFFEE_RECOVERY);
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

// -----------------------------------------------------------------------------
// Stamina & Movement Tier Helpers
// -----------------------------------------------------------------------------
void Player::drainStamina(uint8_t amount) {
    if (stamina > amount) {
        stamina -= amount;
    } else {
        stamina = 0;
    }
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
