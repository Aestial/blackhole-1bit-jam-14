// =============================================================================
// entity.h — Entity Data Structure & Entity Manager
// =============================================================================
//
// PURPOSE:
//   Defines the Entity struct (a single spawnable game object: food or
//   collectible) and the EntityManager that owns a static array of them.
//   The blackhole is NOT an entity — it's managed separately by World.
//
// ARCHITECTURE:
//   This file is 100% PLATFORM-INDEPENDENT. No HAL or Arduboy headers.
//   It only depends on config.h for constants and fixed-point types.
//
// ENTITY LIFECYCLE:
//   1. EntityManager pre-allocates a static array of MAX_ENTITIES entities.
//   2. All start with type = ENTITY_NONE and active = false.
//   3. World::update() calls entityManager.spawn() to activate an entity,
//      setting its type, position, and size.
//   4. Each frame, Game::update() iterates active entities to:
//      - Check collisions with player (physics.h)
//      - Apply blackhole gravity pull (physics.h)
//      - Despawn if too far from camera (entityManager.despawn())
//   5. On collision with player:
//      - FOOD: apply slow debuff (player.applyFoodSlow()), then despawn
//      - COLLECTIBLE: add score, then despawn
//
// ENTITY TYPES:
//   ENTITY_NONE       — Inactive slot (available for spawning)
//   ENTITY_FOOD_PIZZA — Triangle placeholder. Light slow on contact.
//   ENTITY_FOOD_BURGER— Square placeholder. Medium slow on contact.
//   ENTITY_FOOD_DONUT — Circle placeholder. Heavy slow on contact.
//   ENTITY_COLLECTIBLE— Diamond placeholder. Adds score on contact.
//
// FOR IMPLEMENTING AGENTS:
//   - M2: Implement EntityManager::spawn() — find first ENTITY_NONE slot,
//     set type/position/size, mark active. Return false if no slots free.
//   - M2: Implement EntityManager::update() — iterate entities, despawn
//     any that are too far from camera (use World::isOnScreen or distance check).
//   - M2: Implement collision responses in Game::updatePlaying().
//   - Entities are STATIC — they don't move on their own (but get pulled
//     by blackhole gravity via physics::applyBlackholeGravity in M3).
//
// =============================================================================

#ifndef ENTITY_H
#define ENTITY_H

#include "config.h"

// =============================================================================
// EntityType — What kind of entity is this?
// =============================================================================
// Used to determine rendering shape, collision behavior, and food slow values.
// The uint8_t underlying type keeps each entity small.

enum EntityType : uint8_t {
    ENTITY_NONE                 = 0,  // Inactive / empty slot — available for reuse
    // Money / Collectibles (Diamond + Bills)
    ENTITY_COLLECTIBLE_DIAMOND  = 1,  // Faceted Diamond (+10 pts * combo)
    ENTITY_COLLECTIBLE_BILLS    = 2,  // Dollar Bills stack (+25 pts * combo)
    // Power-ups
    ENTITY_POWERUP_COFFEE       = 3,  // Coffee mug (Turbo speed + cleanse for 3s)
    // 8 Distinct Food Hazards (from items_16.png)
    ENTITY_FOOD_APPLE           = 4,  // Apple (Snack: 24 frames, -10% speed)
    ENTITY_FOOD_PIZZA           = 5,  // Pizza slice (Light: 30 frames, -15% speed)
    ENTITY_FOOD_TACO            = 6,  // Taco (Spicy: 45 frames, -25% speed)
    ENTITY_FOOD_BURGER          = 7,  // Burger (Medium: 60 frames, -30% speed)
    ENTITY_FOOD_FRIES           = 8,  // French fries (Salty: 75 frames, -35% speed)
    ENTITY_FOOD_CAKE            = 9,  // Cake (Sugar crash: 105 frames, -45% speed)
    ENTITY_FOOD_DONUT           = 10, // Donut (Heavy: 90 frames, -50% speed)
    ENTITY_FOOD_ICECREAM        = 11, // Ice Cream (Brain freeze: 120 frames, -60% speed)
    // Alias for backwards compatibility
    ENTITY_COLLECTIBLE          = 1
};

// =============================================================================
// Entity — A single game object on the infinite plane
// =============================================================================
// Size: ~14 bytes per entity (important for RAM budget with MAX_ENTITIES slots).
//
// All positions are in WORLD SPACE (fp32_t for large coordinate range).
// To render, convert to screen space: screenX = FP32_TO_INT(x - cameraX)
//
// Width/height are in PIXELS and used for both rendering and AABB collision.
// They're set based on the entity type when spawned (see config.h for sizes).

struct Entity {
    fp32_t x;          // World-space X position (Q24.8 fixed-point)
    fp32_t y;          // World-space Y position (Q24.8 fixed-point)
    EntityType type;   // What kind of entity (determines shape + behavior)
    uint8_t width;     // Hitbox width in pixels
    uint8_t height;    // Hitbox height in pixels
    bool active;       // Is this entity currently alive and on the plane?

    // -------------------------------------------------------------------------
    // reset() — Clear this entity to an inactive state
    // -------------------------------------------------------------------------
    // Called when despawning or at initialization.
    // Sets type to ENTITY_NONE and active to false.
    // Position is zeroed but doesn't matter since inactive entities are skipped.
    void reset();

    // -------------------------------------------------------------------------
    // isFood() — Returns true if this entity is any food type
    // -------------------------------------------------------------------------
    // Convenience for collision handling: if (entity.isFood()) player.applyFoodSlow()
    bool isFood() const;

    // -------------------------------------------------------------------------
    // isCollectible() — Returns true if this entity is a collectible
    // -------------------------------------------------------------------------
    bool isCollectible() const;

    // -------------------------------------------------------------------------
    // isPowerup() — Returns true if this entity is a positive power-up
    // -------------------------------------------------------------------------
    bool isPowerup() const;
};

// =============================================================================
// EntityManager — Static pool manager for all spawned entities
// =============================================================================
// Owns a fixed-size array of Entity structs. Provides spawn/despawn/iteration.
//
// SPAWNING STRATEGY:
//   spawn() scans the array for the first inactive (ENTITY_NONE) slot.
//   If all slots are full, the spawn is silently dropped (returns false).
//   This is intentional — we never exceed MAX_ENTITIES RAM budget.
//
// DESPAWNING STRATEGY:
//   Entities are despawned when they're too far from the camera (off-screen).
//   This is checked by World::update() or Game::updatePlaying() each frame.
//   Also despawned immediately on collision with the player.
//
// ITERATION:
//   Game logic iterates entities[0..MAX_ENTITIES-1] and checks entity.active.
//   Inactive entities are skipped. This is O(MAX_ENTITIES) per frame but
//   MAX_ENTITIES is tiny (12), so it's essentially free.

class EntityManager {
public:
    Entity entities[MAX_ENTITIES];

    // -------------------------------------------------------------------------
    // init() — Reset all entities to inactive state
    // -------------------------------------------------------------------------
    // Called at game start and on retry. Clears the entire entity pool.
    void init();

    // -------------------------------------------------------------------------
    // spawn(type, worldX, worldY) — Spawn a new entity at the given position
    // -------------------------------------------------------------------------
    // Finds the first inactive slot, sets it up with the given type and
    // position. Width/height are set automatically based on type (using
    // constants from config.h: FOOD_PIZZA_SIZE, FOOD_BURGER_SIZE, etc.).
    //
    // Parameters:
    //   type   — EntityType (must not be ENTITY_NONE)
    //   worldX — World-space X position (fp32_t, Q24.8)
    //   worldY — World-space Y position (fp32_t, Q24.8)
    //
    // Returns: true if spawned successfully, false if no slots available.
    //
    // FOR IMPLEMENTING AGENTS (M2):
    //   1. Loop through entities[], find first with type == ENTITY_NONE
    //   2. Set entity.type = type, entity.x = worldX, entity.y = worldY
    //   3. Set entity.active = true
    //   4. Set width/height based on type:
    //      - FOOD_PIZZA:  width=height=FOOD_PIZZA_SIZE
    //      - FOOD_BURGER: width=height=FOOD_BURGER_SIZE
    //      - FOOD_DONUT:  width=height=FOOD_DONUT_SIZE
    //      - COLLECTIBLE: width=height=COLLECTIBLE_SIZE
    //   5. Return true. If no slot found, return false.
    bool spawn(EntityType type, fp32_t worldX, fp32_t worldY);

    // -------------------------------------------------------------------------
    // despawn(index) — Deactivate the entity at the given array index
    // -------------------------------------------------------------------------
    // Calls entity.reset() to mark it as ENTITY_NONE / inactive.
    // The slot becomes available for future spawn() calls.
    //
    // Parameters:
    //   index — Array index (0 to MAX_ENTITIES-1). Must be valid.
    void despawn(uint8_t index);

    // -------------------------------------------------------------------------
    // countActive() — Count the number of currently active entities
    // -------------------------------------------------------------------------
    // Useful for debugging and spawn throttling.
    // Returns: number of entities where active == true.
    uint8_t countActive() const;
};

#endif // ENTITY_H
