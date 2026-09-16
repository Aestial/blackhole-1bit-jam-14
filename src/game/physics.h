// =============================================================================
// physics.h — Collision Detection & Gravity Helpers
// =============================================================================
//
// PURPOSE:
//   Provides physics utility functions used by Game::updatePlaying():
//   - AABB (Axis-Aligned Bounding Box) overlap detection
//   - Blackhole gravitational pull on entities
//   - Movement-toward-target helper (for blackhole chasing player)
//   - Distance approximation (no sqrt — too expensive for AVR)
//
// ARCHITECTURE:
//   100% PLATFORM-INDEPENDENT. Pure math functions.
//   These are standalone utility functions (not a class) because they
//   operate on data owned by Player, Entity, and World.
//
// COLLISION MODEL:
//   All collisions use AABB (rectangle vs rectangle) overlap.
//   Even circular-looking entities (player circle, donut circle) use
//   rectangular hitboxes for simplicity and speed.
//   The hitbox is centered on the entity's (x,y) position.
//
//   Screen-space AABB for an entity at world position (wx, wy) with
//   size (w, h), viewed by camera at (camX, camY):
//     left   = FP32_TO_INT(wx - camX) - w/2
//     top    = FP32_TO_INT(wy - camY) - h/2
//     right  = left + w
//     bottom = top + h
//
//   Two AABBs overlap if:
//     a.left < b.right && a.right > b.left &&
//     a.top < b.bottom && a.bottom > b.top
//
// GRAVITY MODEL (for blackhole pull on entities):
//   Entities within the blackhole's CHARGE RADIUS get pulled toward it.
//   Pull strength is proportional to BH_MASS and inversely proportional
//   to distance (simplified — not real gravity, just "close = strong pull").
//
//   Each frame for each entity within charge radius:
//     direction = normalize(bhPos - entityPos)
//     entity.x += direction.x * BH_MASS * pullFactor
//     entity.y += direction.y * BH_MASS * pullFactor
//   Where pullFactor = (chargeRadius - distance) / chargeRadius (linear falloff)
//
//   This makes entities near the blackhole slide toward it, creating a
//   visual "suction" effect. The player is NOT affected by gravity pull
//   (that would be too frustrating) — only spawned entities are pulled.
//
// DISTANCE APPROXIMATION:
//   True distance = sqrt(dx² + dy²) requires sqrt which is expensive.
//   We use the "Manhattan with correction" approximation:
//     dist ≈ max(|dx|, |dy|) + min(|dx|, |dy|) * 3/8
//   This is accurate to ~3% and uses only integer operations.
//   Fine for game logic — we don't need exact distances.
//
// FOR IMPLEMENTING AGENTS:
//   - M2: Use checkOverlap() for player-entity collisions
//   - M3: Use applyBlackholeGravity() on all entities each frame
//   - M3: Use moveToward() for blackhole tracking player
//   - All functions are implemented here — ready to use in M2/M3
//
// =============================================================================

#ifndef PHYSICS_H
#define PHYSICS_H

#include "config.h"

// =============================================================================
// checkOverlap — AABB collision test between two rectangles
// =============================================================================
// Parameters (all in the SAME coordinate space — either world or screen):
//   ax, ay — Center of rectangle A
//   aw, ah — Width and height of rectangle A (pixels)
//   bx, by — Center of rectangle B
//   bw, bh — Width and height of rectangle B (pixels)
//
// Returns: true if the two rectangles overlap.
//
// NOTE: Positions are fp32_t (Q24.8 world coordinates). The function
//       internally converts to integer pixel coordinates for the test.
//       Width/height are already in pixels (uint8_t).
//
// USAGE EXAMPLE (in Game::updatePlaying):
//   if (checkOverlap(player.x, player.y, player.width, player.height,
//                     entity.x, entity.y, entity.width, entity.height)) {
//       // Collision! Apply food slow or add score
//   }
bool checkOverlap(fp32_t ax, fp32_t ay, uint8_t aw, uint8_t ah,
                   fp32_t bx, fp32_t by, uint8_t bw, uint8_t bh);

// =============================================================================
// approxDistance — Fast distance approximation (no sqrt)
// =============================================================================
// Uses "max + 3/8 min" approximation.
// Parameters: two points in Q24.8 fixed-point world space.
// Returns: approximate distance in Q24.8 fixed-point.
//
// Accuracy: within ~3% of true Euclidean distance. Good enough for
//           charge radius checks and despawn distance tests.
fp32_t approxDistance(fp32_t x0, fp32_t y0, fp32_t x1, fp32_t y1);

// =============================================================================
// moveToward — Move a position toward a target at a given speed
// =============================================================================
// Moves (x, y) toward (targetX, targetY) by at most `speed` units per call.
// Uses normalized direction (approximated) to maintain consistent speed
// regardless of distance.
//
// Parameters:
//   x, y         — Current position (modified in place)
//   targetX, targetY — Target position
//   speed        — Maximum distance to move per call (Q8.8 fixed-point)
//
// This is used by World::update() to move the blackhole toward the player:
//   moveToward(bhX, bhY, player.x, player.y, bhSpeed);
//
// Direction normalization (approximate):
//   dx = targetX - x, dy = targetY - y
//   dist = approxDistance(x, y, targetX, targetY)
//   if (dist > 0):
//     x += dx * speed / dist
//     y += dy * speed / dist
//   (Use FP32 multiplication to avoid overflow)
void moveToward(fp32_t& x, fp32_t& y,
                fp32_t targetX, fp32_t targetY,
                fp_t speed, fp_t dt = FP_DT_ONE);

// =============================================================================
// applyBlackholeGravity — Pull an entity toward the blackhole
// =============================================================================
// If the entity is within chargeRadius of the blackhole, pull it toward
// the blackhole position. Pull strength uses linear falloff:
//   pullFactor = (chargeRadius - distance) / chargeRadius
//   pullForce = mass * pullFactor
//   entity position += direction * pullForce
//
// Parameters:
//   entityX, entityY — Entity position (modified in place if pulled)
//   bhX, bhY         — Blackhole position
//   mass             — Blackhole mass (pull strength, Q8.8)
//   chargeRadius     — How far the gravity reaches (Q24.8 world units)
//   dt               — Normalized delta-time in Q8.8 (default = FP_DT_ONE)
//
// USAGE (in Game::updatePlaying, M3):
//   for each active entity:
//     applyBlackholeGravity(entity.x, entity.y, world.bhX, world.bhY,
//                            world.bhMass, world.bhCharge, dt);
void applyBlackholeGravity(fp32_t& entityX, fp32_t& entityY,
                            fp32_t bhX, fp32_t bhY,
                            fp_t mass, fp32_t chargeRadius,
                            fp_t dt = FP_DT_ONE);

#endif // PHYSICS_H
