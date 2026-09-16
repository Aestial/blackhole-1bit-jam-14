// =============================================================================
// sprites.h — Placeholder Sprite Definitions
// =============================================================================
//
// PURPOSE:
//   Defines visual representation constants for all game entities.
//   During early milestones (M0-M3), entities are drawn using renderer
//   primitives (fillCircle, fillRect, fillTriangle, drawLine).
//   In M5 (polish), these should be replaced with actual PROGMEM bitmap
//   sprite arrays for richer visuals.
//
// PLACEHOLDER SHAPES:
//   Player (fat man)  → Large filled WHITE circle (10x10 px)
//   Pizza             → Filled WHITE triangle (6x6 px)
//   Burger            → Filled WHITE square (6x6 px)
//   Donut             → Small filled WHITE circle (4x4 px, radius 2)
//   Collectible       → Diamond shape (rotated square, 4x4 px)
//   Blackhole         → Concentric circles (outer ring WHITE, inner BLACK)
//
// RENDERING NOTES (for implementing agents):
//   - All shapes are drawn in WHITE (color=1) on BLACK background
//   - The player should be visually distinct (largest shape, filled)
//   - Food items should be smaller than the player but clearly visible
//   - Collectibles should "sparkle" or look different from food (diamond shape)
//   - The blackhole should look ominous — concentric rings suggest depth/gravity
//   - When rendering, use SCREEN coordinates (after world-to-screen transform)
//
// FUTURE PROGMEM SPRITES (M5):
//   When ready to replace primitives with bitmap sprites:
//
//   1. Design sprites in a pixel editor (e.g., Aseprite) at 1-bit depth
//   2. Export as C arrays using a tool like:
//        - TeamARG's Sprite Converter
//        - image2cpp (https://javl.github.io/image2cpp/)
//   3. Store arrays with PROGMEM qualifier:
//        const uint8_t PROGMEM playerSprite[] = { width, height, ... };
//   4. Draw using Sprites::drawOverwrite() or Sprites::drawSelfMasked()
//   5. For rotated sprites (8 directions), pre-render 8 frames in Blender
//      or by hand, store all frames in one array, select frame by direction.
//
// =============================================================================

#ifndef SPRITES_H
#define SPRITES_H

#include <stdint.h>
#include "../src/game/config.h"

// =============================================================================
// PLACEHOLDER RENDERING CONSTANTS
// =============================================================================
// These are used by game.cpp rendering functions to draw each entity type.
// When real sprites are added, these constants may still be used for hitbox sizes.

// Player — Fat man (large filled circle)
static const uint8_t PLAYER_SPRITE_RADIUS = PLAYER_WIDTH / 2;  // 5 pixels

// Pizza — Filled triangle (equilateral-ish, fits in FOOD_PIZZA_SIZE box)
// Vertices relative to center: top, bottom-left, bottom-right
static const int8_t PIZZA_TRI_TOP_Y    = -(FOOD_PIZZA_SIZE / 2);      // -3
static const int8_t PIZZA_TRI_BOTTOM_Y =  (FOOD_PIZZA_SIZE / 2);      //  3
static const int8_t PIZZA_TRI_HALF_W   =  (FOOD_PIZZA_SIZE / 2);      //  3

// Burger — Filled square (FOOD_BURGER_SIZE x FOOD_BURGER_SIZE)
// Drawn as fillRect with offset from center

// Donut — Small filled circle
static const uint8_t DONUT_SPRITE_RADIUS = FOOD_DONUT_SIZE / 2;  // 2 pixels

// Collectible — Diamond (rotated square)
// Drawn as 4 lines forming a diamond shape, or as a fillTriangle pair
static const int8_t COLLECTIBLE_HALF = COLLECTIBLE_SIZE / 2;  // 2 pixels

// Blackhole — Concentric circles
// Outer ring: WHITE circle at BH_RENDER_RADIUS
// Middle ring: BLACK filled circle at BH_RENDER_RADIUS - 2
// Inner ring: WHITE circle at BH_RENDER_RADIUS - 4 (if radius allows)
// Center: BLACK filled circle at 2
// This creates a "tunnel" / "gravity well" visual effect

#endif // SPRITES_H
