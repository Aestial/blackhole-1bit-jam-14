// =============================================================================
// physics.cpp — Collision Detection & Gravity Implementation
// =============================================================================
//
// MILESTONE: M2 (collision detection), M3 (blackhole gravity)
// STATUS: Fully implemented — ready for use by Game in M2/M3.
//
// =============================================================================

#include "physics.h"

// -----------------------------------------------------------------------------
// checkOverlap — AABB collision test
// -----------------------------------------------------------------------------
bool checkOverlap(fp32_t ax, fp32_t ay, uint8_t aw, uint8_t ah,
                   fp32_t bx, fp32_t by, uint8_t bw, uint8_t bh) {
    // Convert world positions to integer pixels for AABB test
    // Hitboxes are centered on (x, y), so offset by half width/height
    int16_t aLeft   = (int16_t)FP32_TO_INT(ax) - (aw / 2);
    int16_t aTop    = (int16_t)FP32_TO_INT(ay) - (ah / 2);
    int16_t aRight  = aLeft + aw;
    int16_t aBottom = aTop  + ah;

    int16_t bLeft   = (int16_t)FP32_TO_INT(bx) - (bw / 2);
    int16_t bTop    = (int16_t)FP32_TO_INT(by) - (bh / 2);
    int16_t bRight  = bLeft + bw;
    int16_t bBottom = bTop  + bh;

    // Two AABBs overlap if they intersect on BOTH axes
    return (aLeft < bRight && aRight > bLeft &&
            aTop < bBottom && aBottom > bTop);
}

// -----------------------------------------------------------------------------
// approxDistance — Fast distance approximation (no sqrt)
// -----------------------------------------------------------------------------
fp32_t approxDistance(fp32_t x0, fp32_t y0, fp32_t x1, fp32_t y1) {
    // Calculate absolute differences
    fp32_t dx = x1 - x0;
    fp32_t dy = y1 - y0;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    // "max + 3/8 min" approximation of Euclidean distance
    // Error: ~3.4% max, good enough for game logic
    fp32_t maxD, minD;
    if (dx > dy) {
        maxD = dx;
        minD = dy;
    } else {
        maxD = dy;
        minD = dx;
    }

    // dist ≈ max + min * 3/8  (using integer shifts: 3/8 ≈ 1/4 + 1/8)
    return maxD + (minD >> 2) + (minD >> 3);
}

// -----------------------------------------------------------------------------
// moveToward — Move position toward target at given speed
// -----------------------------------------------------------------------------
void moveToward(fp32_t& x, fp32_t& y,
                fp32_t targetX, fp32_t targetY,
                fp_t speed) {
    fp32_t dist = approxDistance(x, y, targetX, targetY);

    // If already at target (or very close), don't move
    if (dist < FP_ONE) return;

    // Calculate direction components
    fp32_t dx = targetX - x;
    fp32_t dy = targetY - y;

    // Normalize and scale by speed:
    //   moveX = dx * speed / dist
    //   moveY = dy * speed / dist
    // Use 32-bit intermediate to prevent overflow
    fp32_t moveX = (dx * (fp32_t)speed) / dist;
    fp32_t moveY = (dy * (fp32_t)speed) / dist;

    x += moveX;
    y += moveY;
}

// -----------------------------------------------------------------------------
// applyBlackholeGravity — Pull entity toward blackhole if within charge radius
// -----------------------------------------------------------------------------
void applyBlackholeGravity(fp32_t& entityX, fp32_t& entityY,
                            fp32_t bhX, fp32_t bhY,
                            fp_t mass, fp32_t chargeRadius) {
    fp32_t dist = approxDistance(entityX, entityY, bhX, bhY);

    // Only affect entities within the charge radius
    if (dist >= chargeRadius || dist < FP_ONE) return;

    // Linear falloff: pull is stronger closer to the blackhole
    //   pullFactor = (chargeRadius - dist) / chargeRadius  (0 at edge, 1 at center)
    //   pullForce = mass * pullFactor
    fp32_t pullFactor = chargeRadius - dist;  // numerator (still Q24.8)

    // Direction toward blackhole
    fp32_t dx = bhX - entityX;
    fp32_t dy = bhY - entityY;

    // Apply pull: entity moves toward blackhole
    //   moveX = dx * mass * pullFactor / (dist * chargeRadius)
    // Simplified to avoid overflow:
    //   moveX = dx * mass / dist * pullFactor / chargeRadius
    // Further simplified (mass is small Q8.8, pullFactor/chargeRadius < 1):
    fp32_t moveX = (dx * (fp32_t)mass) / dist;
    fp32_t moveY = (dy * (fp32_t)mass) / dist;

    // Scale by pullFactor / chargeRadius (linear falloff)
    moveX = (moveX * pullFactor) / chargeRadius;
    moveY = (moveY * pullFactor) / chargeRadius;

    entityX += moveX;
    entityY += moveY;
}
