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
    return (type >= ENTITY_FOOD_APPLE && type <= ENTITY_FOOD_ICECREAM);
}

// -----------------------------------------------------------------------------
// Entity::isCollectible()
// -----------------------------------------------------------------------------
bool Entity::isCollectible() const {
    return (type == ENTITY_COLLECTIBLE_DIAMOND || type == ENTITY_COLLECTIBLE_BILLS);
}

// -----------------------------------------------------------------------------
// Entity::isPowerup()
// -----------------------------------------------------------------------------
bool Entity::isPowerup() const {
    return (type == ENTITY_POWERUP_COFFEE);
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
// -----------------------------------------------------------------------------
bool EntityManager::spawn(EntityType type, fp32_t worldX, fp32_t worldY) {
    for (uint8_t i = 0; i < MAX_ENTITIES; i++) {
        if (!entities[i].active || entities[i].type == ENTITY_NONE) {
            entities[i].type = type;
            entities[i].x = worldX;
            entities[i].y = worldY;
            entities[i].active = true;

            switch (type) {
                case ENTITY_COLLECTIBLE_DIAMOND:
                case ENTITY_COLLECTIBLE_BILLS:
                    entities[i].width = COLLECTIBLE_SIZE;
                    entities[i].height = COLLECTIBLE_SIZE;
                    break;
                case ENTITY_POWERUP_COFFEE:
                    entities[i].width = POWERUP_COFFEE_SIZE;
                    entities[i].height = POWERUP_COFFEE_SIZE;
                    break;
                case ENTITY_FOOD_APPLE:
                case ENTITY_FOOD_PIZZA:
                case ENTITY_FOOD_TACO:
                case ENTITY_FOOD_BURGER:
                case ENTITY_FOOD_FRIES:
                case ENTITY_FOOD_CAKE:
                case ENTITY_FOOD_DONUT:
                case ENTITY_FOOD_ICECREAM:
                    entities[i].width = 8;
                    entities[i].height = 8;
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
