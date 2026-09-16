// =============================================================================
// world.cpp — World State Implementation
// =============================================================================
//
// MILESTONES:
//   M1: Camera tracking (implemented)
//   M2: Spawning + despawning (spawn timer implemented, actual spawn in Game)
//   M3: Blackhole movement + difficulty ramping (implemented)
//   M4: Background grid rendering (done in Game::renderBackground)
//
// =============================================================================

#include "world.h"
#include "physics.h"  // For moveToward()

// -----------------------------------------------------------------------------
// World::init()
// -----------------------------------------------------------------------------
void World::init() {
    camX = 0;
    camY = 0;

    // Blackhole starts to the left of the player, at BH_START_DISTANCE
    bhX = INT_TO_FP32(-BH_START_DISTANCE);
    bhY = 0;
    bhSpeed = BH_BASE_SPEED;
    bhMass = BH_MASS;
    bhCharge = BH_BASE_CHARGE;
    bhSpin = 0;

    gameTime = 0;
    spawnTimer = SPAWN_INTERVAL_START;
    spawnInterval = SPAWN_INTERVAL_START;
    scoreTimer = SCORE_TIME_INTERVAL;
}

// -----------------------------------------------------------------------------
// World::update()
// -----------------------------------------------------------------------------
void World::update(fp32_t playerX, fp32_t playerY) {
    // 1. Update camera — directly follow player (no smoothing for now)
    camX = playerX;
    camY = playerY;

    // 2. Move blackhole toward player
    //    TODO(M3): Uncomment when physics.h moveToward is integrated:
    moveToward(bhX, bhY, playerX, playerY, bhSpeed);

    // 3. Spin (cosmetic — wraps at 255 automatically for uint8_t)
    bhSpin++;

    // 4. Ramp difficulty (linear increase over time)
    //    bhSpeed increases by a tiny amount each frame (BH_SPEED_INCREMENT = 1 in Q8.8 = 1/256)
    bhSpeed += BH_SPEED_INCREMENT;

    //    bhCharge (attraction radius) also increases
    bhCharge += BH_CHARGE_INCREMENT;

    //    Spawn interval decreases every SPAWN_RAMP_INTERVAL frames
    if (gameTime > 0 && (gameTime % SPAWN_RAMP_INTERVAL) == 0) {
        if (spawnInterval > SPAWN_MIN_INTERVAL) {
            spawnInterval--;
        }
    }

    // 5. Decrement spawn timer
    if (spawnTimer > 0) {
        spawnTimer--;
    }

    // 6. Increment game time
    gameTime++;

    // 7. Decrement score timer
    if (scoreTimer > 0) {
        scoreTimer--;
    }
}

// -----------------------------------------------------------------------------
// World::shouldSpawn()
// -----------------------------------------------------------------------------
bool World::shouldSpawn() const {
    return spawnTimer == 0;
}

// -----------------------------------------------------------------------------
// World::resetSpawnTimer()
// -----------------------------------------------------------------------------
void World::resetSpawnTimer() {
    spawnTimer = spawnInterval;
}

// -----------------------------------------------------------------------------
// World::shouldScoreTick()
// -----------------------------------------------------------------------------
bool World::shouldScoreTick() const {
    return scoreTimer == 0;
}

// -----------------------------------------------------------------------------
// World::resetScoreTimer()
// -----------------------------------------------------------------------------
void World::resetScoreTimer() {
    scoreTimer = SCORE_TIME_INTERVAL;
}

// -----------------------------------------------------------------------------
// World::worldToScreenX()
// -----------------------------------------------------------------------------
int16_t World::worldToScreenX(fp32_t worldX) const {
    return (int16_t)FP32_TO_INT(worldX - camX) + (SCREEN_W / 2);
}

// -----------------------------------------------------------------------------
// World::worldToScreenY()
// -----------------------------------------------------------------------------
int16_t World::worldToScreenY(fp32_t worldY) const {
    return (int16_t)FP32_TO_INT(worldY - camY) + (SCREEN_H / 2);
}

// -----------------------------------------------------------------------------
// World::isOnScreen()
// -----------------------------------------------------------------------------
bool World::isOnScreen(fp32_t worldX, fp32_t worldY, uint8_t w, uint8_t h) const {
    int16_t sx = worldToScreenX(worldX);
    int16_t sy = worldToScreenY(worldY);

    // Check if the entity's bounding box overlaps the screen
    return (sx + w / 2 >= 0 && sx - w / 2 < SCREEN_W &&
            sy + h / 2 >= 0 && sy - h / 2 < SCREEN_H);
}

// -----------------------------------------------------------------------------
// World::isTooFar()
// -----------------------------------------------------------------------------
bool World::isTooFar(fp32_t worldX, fp32_t worldY) const {
    int16_t sx = worldToScreenX(worldX);
    int16_t sy = worldToScreenY(worldY);

    // Entity is "too far" if it's more than DESPAWN_DISTANCE from any screen edge
    return (sx < -(int16_t)DESPAWN_DISTANCE ||
            sx > (int16_t)(SCREEN_W + DESPAWN_DISTANCE) ||
            sy < -(int16_t)DESPAWN_DISTANCE ||
            sy > (int16_t)(SCREEN_H + DESPAWN_DISTANCE));
}
