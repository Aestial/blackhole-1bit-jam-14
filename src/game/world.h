// =============================================================================
// world.h — World State: Camera, Blackhole, Spawning, Difficulty
// =============================================================================
//
// PURPOSE:
//   Manages the "world" — everything that exists on the infinite plane
//   beyond the player: the camera, the blackhole entity, entity spawning,
//   difficulty ramping, and background grid state.
//
// ARCHITECTURE:
//   100% PLATFORM-INDEPENDENT. No HAL, no Arduboy headers.
//   Rendering of the world (background, blackhole, entities) is handled
//   by Game's render methods in game.cpp, which reads World's data.
//
// THE BLACKHOLE:
//   The blackhole is NOT in the EntityManager — it's a special singleton
//   managed here. It has 3 translated real-world properties:
//
//   - MASS (bhMass, fp_t):
//     Controls gravitational pull strength on nearby entities.
//     Stays constant throughout the game (BH_MASS from config.h).
//     Higher mass = entities get sucked in faster.
//
//   - SPIN (bhSpin, uint8_t):
//     Cosmetic rotation counter. Increments each frame, wraps at 255.
//     Used by Game::renderBlackhole() to animate concentric rings.
//     Has NO gameplay effect — purely visual.
//
//   - CHARGE (bhCharge, fp32_t):
//     The attraction radius — how far the blackhole's gravity reaches.
//     Starts at BH_BASE_CHARGE and LINEARLY INCREASES over time.
//     Entities within this radius get pulled toward the blackhole.
//     Higher charge = bigger danger zone.
//
//   The blackhole also TRANSLATES (moves) toward the player each frame
//   at bhSpeed, which also linearly increases over time.
//
// CAMERA:
//   Camera position tracks the player. The camera defines what portion
//   of the infinite world is visible on the 128x64 screen.
//   Camera (camX, camY) = player position offset so player is at screen center.
//
//   World-to-screen conversion:
//     screenX = FP32_TO_INT(worldX - camX) + SCREEN_W / 2
//     screenY = FP32_TO_INT(worldY - camY) + SCREEN_H / 2
//   (Player is always at screen center: SCREEN_W/2, SCREEN_H/2)
//
// SPAWNING:
//   Every spawnInterval frames, World::update() signals that a new entity
//   should be spawned. The Game class calls entityManager.spawn() with a
//   random type and position.
//
//   Spawn position: random point within SPAWN_RADIUS of the player,
//   but at least SPAWN_MIN_DISTANCE away (so items don't appear on top).
//   Entity type chosen randomly: ~60% food (20% each type), ~40% collectible.
//
//   spawnInterval decreases by 1 every SPAWN_RAMP_INTERVAL frames,
//   down to SPAWN_MIN_INTERVAL. This makes the plane busier over time.
//
// DESPAWNING:
//   Entities too far from the camera (> DESPAWN_DISTANCE from screen edge)
//   are despawned to free their EntityManager slot for new spawns.
//
// DIFFICULTY RAMPING (all linear):
//   - bhSpeed increases by BH_SPEED_INCREMENT per frame
//   - bhCharge increases by BH_CHARGE_INCREMENT per frame
//   - spawnInterval decreases every SPAWN_RAMP_INTERVAL frames
//   This ensures the game gets progressively harder with no respite.
//
// FOR IMPLEMENTING AGENTS:
//   - M1: Implement camera tracking (simple: camX = player.x, camY = player.y)
//   - M2: Implement spawning logic (spawnTimer countdown, random position/type)
//   - M2: Implement despawning (iterate entities, check distance from camera)
//   - M3: Implement blackhole movement (moveToward from physics.h)
//   - M3: Implement difficulty ramping (increment bhSpeed, bhCharge, decrement spawnInterval)
//   - M4: Implement background grid rendering in Game::renderBackground()
//
// =============================================================================

#ifndef WORLD_H
#define WORLD_H

#include "config.h"
#include "entity.h"

// =============================================================================
// World — Global game world state
// =============================================================================
// Size: ~50 bytes. One instance owned by Game.

struct World {
    // ---- Camera (follows player, defines visible region) ----
    fp32_t camX;   // Camera world X position (player-centered)
    fp32_t camY;   // Camera world Y position (player-centered)

    // ---- Blackhole entity (singleton, not in EntityManager) ----
    fp32_t bhX;         // Blackhole world X position
    fp32_t bhY;         // Blackhole world Y position
    fp_t   bhSpeed;     // Current chase speed (increases over time)
    fp_t   bhMass;      // Gravitational pull strength (constant: BH_MASS)
    fp32_t bhCharge;    // Attraction radius (increases over time)
    uint8_t bhSpin;     // Visual spin counter (0-255, wraps, cosmetic only)

    // ---- Timing & difficulty ----
    uint16_t gameTime;      // Total frames elapsed since game start
    uint8_t  spawnTimer;    // Frames until next entity spawn
    uint8_t  spawnInterval; // Current frames between spawns (decreases over time)
    uint16_t scoreTimer;    // Frames until next passive score tick
    fp_t     timeAccum;     // Fractional accumulator for delta-time

    // =========================================================================
    // init() — Reset world state for a new game
    // =========================================================================
    // Called at game start and on retry.
    // Sets camera to origin, blackhole to starting position, resets timers.
    //
    // Initial state:
    //   camX = camY = 0
    //   bhX = -BH_START_DISTANCE (blackhole starts to the left of player)
    //   bhY = 0
    //   bhSpeed = BH_BASE_SPEED
    //   bhMass = BH_MASS
    //   bhCharge = BH_BASE_CHARGE
    //   bhSpin = 0
    //   gameTime = 0
    //   spawnTimer = SPAWN_INTERVAL_START
    //   spawnInterval = SPAWN_INTERVAL_START
    //   scoreTimer = SCORE_TIME_INTERVAL
    void init();

    // =========================================================================
    // update(playerX, playerY, dt) — Per-frame world update
    // =========================================================================
    // Called once per frame during STATE_PLAYING.
    //
    // Parameters:
    //   playerX, playerY — Current player world position (for camera + blackhole tracking)
    //   dt               — Normalized delta-time in Q8.8 (default = FP_DT_ONE)
    //
    void update(fp32_t playerX, fp32_t playerY, fp_t dt = FP_DT_ONE);

    // =========================================================================
    // shouldSpawn() — Check if it's time to spawn a new entity
    // =========================================================================
    // Returns true if spawnTimer has reached 0.
    // After returning true, the caller should reset spawnTimer to spawnInterval.
    bool shouldSpawn() const;

    // =========================================================================
    // resetSpawnTimer() — Reset spawn timer after a successful spawn
    // =========================================================================
    // Sets spawnTimer = spawnInterval.
    void resetSpawnTimer();

    // =========================================================================
    // shouldScoreTick() — Check if it's time for a passive score increment
    // =========================================================================
    // Returns true if scoreTimer has reached 0.
    // Caller should add SCORE_PER_TICK to score and call resetScoreTimer().
    bool shouldScoreTick() const;

    // =========================================================================
    // resetScoreTimer() — Reset score timer after a tick
    // =========================================================================
    void resetScoreTimer();

    // =========================================================================
    // worldToScreenX / worldToScreenY — Convert world coords to screen coords
    // =========================================================================
    // Used by Game render methods to position sprites on screen.
    // The camera is centered on the player (screen center = SCREEN_W/2, SCREEN_H/2).
    //
    //   screenX = FP32_TO_INT(worldX - camX) + SCREEN_W / 2
    //   screenY = FP32_TO_INT(worldY - camY) + SCREEN_H / 2
    //
    int16_t worldToScreenX(fp32_t worldX) const;
    int16_t worldToScreenY(fp32_t worldY) const;

    // =========================================================================
    // isOnScreen(worldX, worldY, w, h) — Check if an entity is visible
    // =========================================================================
    // Returns true if an entity centered at (worldX, worldY) with size (w, h)
    // is at least partially within the screen bounds.
    // Used for render culling and despawn checks.
    //
    // An entity is on screen if its screen-space AABB overlaps [0, 0, SCREEN_W, SCREEN_H].
    bool isOnScreen(fp32_t worldX, fp32_t worldY, uint8_t w, uint8_t h) const;

    // =========================================================================
    // isTooFar(worldX, worldY) — Check if entity should be despawned
    // =========================================================================
    // Returns true if the entity is more than DESPAWN_DISTANCE pixels
    // from the nearest screen edge (in screen space).
    // This is a wider check than isOnScreen — we don't despawn immediately
    // when off-screen, only when well outside the visible area.
    bool isTooFar(fp32_t worldX, fp32_t worldY) const;
};

#endif // WORLD_H
