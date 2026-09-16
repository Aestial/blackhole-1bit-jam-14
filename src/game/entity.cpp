// =============================================================================
// entity.cpp — Entity & EntityManager Implementation
// =============================================================================
//
// MILESTONE: M2 (Entity Spawning & Collision Detection)
//
// FOR IMPLEMENTING AGENTS:
//   Implement all methods as documented in entity.h.
//   Key points:
//   - spawn() must find first ENTITY_NONE slot and configure it
//   - despawn() just calls entity.reset()
//   - Keep it simple — entities are static data, logic lives in Game/Physics
//
// =============================================================================

#include "entity.h"

// -----------------------------------------------------------------------------
// Entity::reset()
// -----------------------------------------------------------------------------
void Entity::reset() {
    x = 0;
    y = 0;
    type = ENTITY_NONE;
    width = 0;
    height = 0;
    active = false;
}

// -----------------------------------------------------------------------------
// Entity::isFood()
// -----------------------------------------------------------------------------
bool Entity::isFood() const {
    return (type == ENTITY_FOOD_PIZZA ||
            type == ENTITY_FOOD_BURGER ||
            type == ENTITY_FOOD_DONUT);
}

// -----------------------------------------------------------------------------
// Entity::isCollectible()
// -----------------------------------------------------------------------------
bool Entity::isCollectible() const {
    return (type == ENTITY_COLLECTIBLE);
}

// -----------------------------------------------------------------------------
// EntityManager::init()
// -----------------------------------------------------------------------------
void EntityManager::init() {
    for (uint8_t i = 0; i < MAX_ENTITIES; i++) {
        entities[i].reset();
    }
}

// -----------------------------------------------------------------------------
// EntityManager::spawn()
// TODO(M2): Implement — find first inactive slot, set type/pos/size, activate
// -----------------------------------------------------------------------------
bool EntityManager::spawn(EntityType type, fp32_t worldX, fp32_t worldY) {
    // TODO(M2): Implement entity spawning
    // 1. Loop through entities[] to find first slot where type == ENTITY_NONE
    // 2. If found:
    //    a. Set entity.type = type
    //    b. Set entity.x = worldX, entity.y = worldY
    //    c. Set entity.active = true
    //    d. Set width/height based on type:
    //       - ENTITY_FOOD_PIZZA:  w=h=FOOD_PIZZA_SIZE (6)
    //       - ENTITY_FOOD_BURGER: w=h=FOOD_BURGER_SIZE (6)
    //       - ENTITY_FOOD_DONUT:  w=h=FOOD_DONUT_SIZE (4)
    //       - ENTITY_COLLECTIBLE: w=h=COLLECTIBLE_SIZE (4)
    //    e. Return true
    // 3. If no slot found, return false (pool full — silently drop spawn)

    for (uint8_t i = 0; i < MAX_ENTITIES; i++) {
        if (entities[i].type == ENTITY_NONE) {
            entities[i].type = type;
            entities[i].x = worldX;
            entities[i].y = worldY;
            entities[i].active = true;

            switch (type) {
                case ENTITY_FOOD_PIZZA:
                    entities[i].width = FOOD_PIZZA_SIZE;
                    entities[i].height = FOOD_PIZZA_SIZE;
                    break;
                case ENTITY_FOOD_BURGER:
                    entities[i].width = FOOD_BURGER_SIZE;
                    entities[i].height = FOOD_BURGER_SIZE;
                    break;
                case ENTITY_FOOD_DONUT:
                    entities[i].width = FOOD_DONUT_SIZE;
                    entities[i].height = FOOD_DONUT_SIZE;
                    break;
                case ENTITY_COLLECTIBLE:
                    entities[i].width = COLLECTIBLE_SIZE;
                    entities[i].height = COLLECTIBLE_SIZE;
                    break;
                default:
                    entities[i].reset();
                    return false;
            }
            return true;
        }
    }
    return false;
}

// -----------------------------------------------------------------------------
// EntityManager::despawn()
// -----------------------------------------------------------------------------
void EntityManager::despawn(uint8_t index) {
    if (index < MAX_ENTITIES) {
        entities[index].reset();
    }
}

// -----------------------------------------------------------------------------
// EntityManager::countActive()
// -----------------------------------------------------------------------------
uint8_t EntityManager::countActive() const {
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_ENTITIES; i++) {
        if (entities[i].active) {
            count++;
        }
    }
    return count;
}
