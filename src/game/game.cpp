// =============================================================================
// game.cpp — Game State Machine & Rendering Implementation
// =============================================================================
//
// MILESTONES:
//   M0: Skeleton with state machine and basic stubs (current)
//   M1: renderPlayer(), renderBackground() (basic scrolling grid)
//   M2: updatePlaying() (spawning, collision, scoring), renderEntities()
//   M3: Blackhole gravity in updatePlaying(), renderBlackhole()
//   M4: renderTitle(), renderGameOver(), renderHUD(), converging grid
//
// FOR IMPLEMENTING AGENTS:
//   This file has working state transitions and basic stubs.
//   Each render/update method has a detailed TODO block describing
//   exactly what to implement. Follow the comments in game.h for specs.
//
// =============================================================================

#include "game.h"
#include "physics.h"
#include "../../assets/sprites.h"

// =============================================================================
// init() — One-time setup
// =============================================================================
void Game::init(HalStorage& storage) {
    storageRef = &storage;
    highScore = storage.loadHighScore();
    state = STATE_TITLE;
    score = 0;
    comboMultiplier = 1;
}

// =============================================================================
// update() — Per-frame dispatch
// =============================================================================
void Game::update(HalInput& input, fp_t dt) {
    switch (state) {
        case STATE_TITLE:
            updateTitle(input, dt);
            break;
        case STATE_PLAYING:
            updatePlaying(input, dt);
            break;
        case STATE_GAMEOVER:
            updateGameOver(input, dt);
            break;
    }
}

// =============================================================================
// render() — Per-frame dispatch
// =============================================================================
void Game::render(HalRenderer& renderer) {
    switch (state) {
        case STATE_TITLE:
            renderTitle(renderer);
            break;
        case STATE_PLAYING:
            renderPlaying(renderer);
            break;
        case STATE_GAMEOVER:
            renderGameOver(renderer);
            break;
    }
}

// =============================================================================
// reset() — Reset game state for a new game
// =============================================================================
void Game::reset() {
    player.init();
    world.init();
    entities.init();
    score = 0;
    comboMultiplier = 1;

    // M3: Initialize accretion particles
    for (uint8_t i = 0; i < MAX_GRAVITY_PARTICLES; i++) {
        particles[i].life = 0;
    }
    particleSpawnTimer = 0;

    // Pre-populate 2 spacious items across the plane:
    // 1. Diamond in the whitehole risk-reward orbit (52px from whitehole, safe from 30px charge)
    // 2. Pizza hazard ahead in the flight path (>70px away from Diamond)
    entities.spawn(ENTITY_COLLECTIBLE_DIAMOND, world.bhX + INT_TO_FP32(50), world.bhY - INT_TO_FP32(15));
    entities.spawn(ENTITY_FOOD_PIZZA, player.x + INT_TO_FP32(25), player.y - INT_TO_FP32(65));
}

// =============================================================================
// STATE: TITLE
// =============================================================================

void Game::updateTitle(HalInput& input, fp_t dt) {
    world.timeAccum += dt;
    while (world.timeAccum >= FP_DT_ONE) {
        world.timeAccum -= FP_DT_ONE;
        world.bhSpin++;
    }
    if (input.justPressed(BTN_A)) {
        reset();
        state = STATE_PLAYING;
    }
}

void Game::renderTitle(HalRenderer& renderer) {
    // 1. Full-screen cover image (128x64) with static whitehole blanked
    renderer.drawSelfMasked(0, 0, title_screen_sprite, 0);

    // 2. Animated swirling whitehole at bottom-right (x=92, y=30)
    // Slower, majestic rotation using WHITEHOLE_ANIM_DIVISOR
    uint8_t frame = (world.bhSpin / WHITEHOLE_ANIM_DIVISOR) % WHITEHOLE_FRAME_COUNT;
    renderer.drawSelfMasked(TITLE_WHITEHOLE_X, TITLE_WHITEHOLE_Y, whitehole_sprite, frame);

    // 3. High score in the open bottom-left area (smaller standard font per user request)
    renderer.setCursor(6, 38);
    renderer.print("High: ");
    renderer.printNumber(highScore);

    // 4. "Press A" prompt in the open bottom-left area (custom bold title font)
    drawTitleText(renderer, 6, 50, "Press A");
}

// =============================================================================
// STATE: PLAYING
// =============================================================================

void Game::updatePlaying(HalInput& input, fp_t dt) {
    // 1. Read input
    bool up    = input.pressed(BTN_UP);
    bool down  = input.pressed(BTN_DOWN);
    bool left  = input.pressed(BTN_LEFT);
    bool right = input.pressed(BTN_RIGHT);
    bool accel = input.pressed(BTN_A);
    bool brake = input.pressed(BTN_B);

    // 2. Update player movement with delta time
    player.update(up, down, left, right, accel, brake, dt);

    // 3. Update world (camera, blackhole, difficulty, timers) with delta time
    world.update(player.x, player.y, dt);

    // 4. Spawn entities
    if (world.shouldSpawn()) {
        // Roll for entity type:
        //   30% Diamond (+10 pts)
        //   10% Dollar Bills (+25 pts)
        //   15% Coffee powerup (speed boost + cleanse)
        //   45% Food hazards (split evenly across 8 food hazards)
        uint16_t roll = world.randomRange(0, 99);
        EntityType spawnType;
        if (roll < 30) {
            spawnType = ENTITY_COLLECTIBLE_DIAMOND;
        } else if (roll < 40) {
            spawnType = ENTITY_COLLECTIBLE_BILLS;
        } else if (roll < 55) {
            spawnType = ENTITY_POWERUP_COFFEE;
        } else {
            // Food hazards (8 distinct types: APPLE through ICECREAM)
            uint8_t foodRoll = world.randomRange(0, 7);
            spawnType = (EntityType)(ENTITY_FOOD_APPLE + foodRoll);
        }

        bool isMoney = (spawnType == ENTITY_COLLECTIBLE_DIAMOND || spawnType == ENTITY_COLLECTIBLE_BILLS);

        // Try candidate positions enforcing MIN_ITEM_SEPARATION and whitehole attraction safety
        for (uint8_t attempt = 0; attempt < 4; attempt++) {
            fp32_t wx = 0;
            fp32_t wy = 0;

            // Money spawns primarily (80%) near the whitehole with a decaying distribution curve,
            // but safely outside its attraction force. On attempt 3, fallback to player path.
            bool spawnNearWhitehole = isMoney && (attempt < 3) && (world.randomRange(0, 99) < MONEY_BH_SPAWN_CHANCE);

            if (spawnNearWhitehole) {
                // Whitehole Risk-Reward Zone:
                // Safe distance starts beyond whitehole attraction radius (bhCharge) + safety buffer
                // so that neither the item nor the player gets sucked in by the whitehole.
                uint16_t minSafeDist = (uint16_t)FP_TO_INT(world.bhCharge) + MONEY_BH_SAFE_BUFFER;

                // Decaying distribution curve peaked at minSafeDist:
                // min(u1, u2) produces a linearly decaying density function:
                // 75% of items spawn in the inner half of the orbit ring!
                uint16_t u1 = world.randomRange(0, MONEY_BH_RING_SPAN);
                uint16_t u2 = world.randomRange(0, MONEY_BH_RING_SPAN);
                uint16_t offset = (u1 < u2) ? u1 : u2;
                int16_t r = (int16_t)(minSafeDist + offset);

                // Sample isotropic angle from 16-point unit circle lookup table
                uint8_t dir = world.nextRandom() & 15;
                int16_t dx = (int16_t)(((int32_t)UNIT_CIRCLE_X[dir] * r) / 127);
                int16_t dy = (int16_t)(((int32_t)UNIT_CIRCLE_Y[dir] * r) / 127);

                wx = world.bhX + INT_TO_FP32(dx);
                wy = world.bhY + INT_TO_FP32(dy);

                // Guard: ensure not right on top of player (< 35px)
                if (approxDistance(wx, wy, player.x, player.y) < INT_TO_FP32(35)) {
                    continue;
                }

                // Guard: ensure not out of camera despawn range
                if (world.isTooFar(wx, wy)) {
                    continue;
                }
            } else {
                // Standard flight ring around player (used for food hazards, coffee, and open-field money)
                int16_t dist = (int16_t)world.randomRange(SPAWN_MIN_DISTANCE, SPAWN_RADIUS);
                int16_t dx = (int16_t)world.randomRange(0, dist);
                int16_t dy = dist - dx;
                if (world.nextRandom() & 1) dx = -dx;
                if (world.nextRandom() & 1) dy = -dy;

                // Bias 60% of spawns ahead in the player's movement direction
                if ((player.vx != 0 || player.vy != 0) && (world.nextRandom() % 5 < 3)) {
                    if (player.vx > 0 && dx < 0) dx = -dx;
                    if (player.vx < 0 && dx > 0) dx = -dx;
                    if (player.vy > 0 && dy < 0) dy = -dy;
                    if (player.vy < 0 && dy > 0) dy = -dy;
                }

                wx = player.x + INT_TO_FP32(dx);
                wy = player.y + INT_TO_FP32(dy);

                // Guard: do not spawn inside the whitehole core or attraction radius
                uint16_t minSafe = (uint16_t)FP_TO_INT(world.bhCharge) + 10;
                if (approxDistance(wx, wy, world.bhX, world.bhY) <= INT_TO_FP32(minSafe)) {
                    continue;
                }
            }

            // Check separation against all existing active entities
            bool tooClose = false;
            for (uint8_t i = 0; i < MAX_ENTITIES; i++) {
                if (entities.entities[i].active) {
                    if (approxDistance(wx, wy, entities.entities[i].x, entities.entities[i].y) < INT_TO_FP32(MIN_ITEM_SEPARATION)) {
                        tooClose = true;
                        break;
                    }
                }
            }

            if (!tooClose) {
                entities.spawn(spawnType, wx, wy);
                break;
            }
        }

        world.resetSpawnTimer();
    }

    // 5. Apply blackhole gravity to entities (M3: enabled)
    for (uint8_t i = 0; i < MAX_ENTITIES; i++) {
        if (entities.entities[i].active) {
            applyBlackholeGravity(entities.entities[i].x, entities.entities[i].y,
                                   world.bhX, world.bhY,
                                   world.bhMass, world.bhCharge, dt);

            // M3: Absorb entities that fall into the whitehole core
            if (approxDistance(entities.entities[i].x, entities.entities[i].y,
                               world.bhX, world.bhY) < INT_TO_FP32(BH_ABSORB_RADIUS)) {
                entities.despawn(i);
            }
        }
    }

    // 5b. Apply soft gravity pull on the player (M3: 25% of entity strength)
    {
        fp32_t dist = approxDistance(player.x, player.y, world.bhX, world.bhY);
        if (dist < world.bhCharge && dist > INT_TO_FP32(BH_ABSORB_RADIUS)) {
            fp32_t pullFactor = world.bhCharge - dist;
            fp32_t dx = world.bhX - player.x;
            fp32_t dy = world.bhY - player.y;
            fp_t effMass = FP_MUL(FP_MUL(world.bhMass, BH_PLAYER_GRAVITY_SCALE), dt);
            fp32_t moveX = (dx * (fp32_t)effMass) / dist;
            fp32_t moveY = (dy * (fp32_t)effMass) / dist;
            moveX = (moveX * pullFactor) / world.bhCharge;
            moveY = (moveY * pullFactor) / world.bhCharge;
            player.x += moveX;
            player.y += moveY;
        }
    }

    // 5c. Update accretion particles (M3)
    updateParticles(dt);

    // 6. Check player-entity collisions
    for (uint8_t i = 0; i < MAX_ENTITIES; i++) {
        if (!entities.entities[i].active) continue;
        Entity& e = entities.entities[i];
        if (checkOverlap(player.x, player.y, player.width, player.height,
                          e.x, e.y, e.width, e.height)) {
            if (e.isFood()) {
                player.applyFoodSlow(e.type);
                comboMultiplier = 1; // Food hazard interrupts combo streak
            } else if (e.isCollectible()) {
                uint8_t baseScore = (e.type == ENTITY_COLLECTIBLE_BILLS) ? SCORE_PER_BILLS : SCORE_PER_DIAMOND;
                score += (uint16_t)baseScore * comboMultiplier;
                if (comboMultiplier < COMBO_MAX) {
                    comboMultiplier++;
                }
            } else if (e.isPowerup()) {
                player.applyCoffeeBoost();
                // Power-up also rewards combo streak
                if (comboMultiplier < COMBO_MAX) {
                    comboMultiplier++;
                }
            }
            entities.despawn(i);
        }
    }

    // 7. Despawn far entities
    for (uint8_t i = 0; i < MAX_ENTITIES; i++) {
        if (entities.entities[i].active && world.isTooFar(entities.entities[i].x, entities.entities[i].y)) {
            entities.despawn(i);
        }
    }

    // 8. Passive score
    if (world.shouldScoreTick()) {
        score += SCORE_PER_TICK;
        world.resetScoreTimer();
    }

    // 9. Check game over (blackhole catches player)
    if (checkOverlap(player.x, player.y, player.width, player.height,
                      world.bhX, world.bhY,
                      BH_RENDER_RADIUS * 2, BH_RENDER_RADIUS * 2)) {
        state = STATE_GAMEOVER;
        if (score > highScore) {
            highScore = score;
            if (storageRef) {
                storageRef->saveHighScore(highScore);
            }
        }
    }
}

void Game::renderPlaying(HalRenderer& renderer) {
    // Draw order: back → front (painter's algorithm)
    renderBackground(renderer);
    renderEntities(renderer);
    renderBlackhole(renderer);
    renderParticles(renderer);  // M3: accretion disk particles
    renderPlayer(renderer);
    renderHUD(renderer);
}

// =============================================================================
// STATE: GAME OVER
// =============================================================================

void Game::updateGameOver(HalInput& input, fp_t dt) {
    (void)dt;
    if (input.justPressed(BTN_A)) {
        state = STATE_TITLE;
    } else if (input.justPressed(BTN_B)) {
        reset();
        state = STATE_PLAYING;
    }
}

void Game::renderGameOver(HalRenderer& renderer) {
    // TODO(M4): Fancy game over screen with animation
    // For now: simple text display

    // "GAME OVER" — 9 chars × 6px = 54px → x = (128-54)/2 = 37
    renderer.setCursor(37, 8);
    renderer.print("GAME OVER");

    renderer.setCursor(22, 24);
    renderer.print("Score:");
    renderer.printNumber(score);

    renderer.setCursor(22, 36);
    renderer.print("High:");
    renderer.printNumber(highScore);

    // "A:Menu B:Retry" — 14 chars × 6px = 84px → x = (128-84)/2 = 22
    renderer.setCursor(22, 52);
    renderer.print("A:Menu B:Retry");
}

// =============================================================================
// RENDER COMPONENTS
// =============================================================================

void Game::renderBackground(HalRenderer& renderer) {
    // =========================================================================
    // Pseudo-3D Perspective Ground Grid with Spacetime Curvature (M4)
    // =========================================================================
    // The vanishing point (HORIZON_Y) is above the screen, so the entire
    // 128×64 display is ground plane. Grid lines are distorted toward the
    // whitehole position, creating a gravitational lensing / spacetime
    // curvature visual effect. Distortion scales with bhCharge over time.

    // 1. Horizon line (only drawn if vanishing point is on-screen)
    if (HORIZON_Y >= 0) {
        renderer.drawLine(0, HORIZON_Y, SCREEN_W - 1, HORIZON_Y, COLOR_WHITE);
    }

    // 2. Compute whitehole screen position for grid distortion (M4)
    int16_t bhSX = world.worldToScreenX(world.bhX, world.bhY);
    int16_t bhSY = world.worldToScreenY(world.bhY);

    // Distortion strength scales with bhCharge (grows over time)
    // Visible from the start but subtle — DIVISOR keeps initial values low
    int16_t distortStrength = (int16_t)(FP_TO_INT(world.bhCharge) / GRID_DISTORT_DIVISOR);
    if (distortStrength > GRID_DISTORT_MAX_STRENGTH) {
        distortStrength = GRID_DISTORT_MAX_STRENGTH;
    }
    int16_t distortRadiusSq = (int16_t)GRID_DISTORT_RADIUS * GRID_DISTORT_RADIUS;

    // 3. Perspective rays converging toward vanishing point (scrolling with camX)
    //    M4: Ray endpoints are distorted toward the whitehole (gravitational lensing)
    int16_t camX_int = (int16_t)FP32_TO_INT(world.camX);
    int16_t xOffset = camX_int % BASE_SPACING_X;
    if (xOffset < 0) xOffset += BASE_SPACING_X;

    for (int16_t bx = -xOffset - BASE_SPACING_X * 2;
         bx <= SCREEN_W + BASE_SPACING_X * 2;
         bx += BASE_SPACING_X) {

        int16_t tx = (SCREEN_W / 2) + ((bx - (SCREEN_W / 2)) * TOP_SPACING_X) / BASE_SPACING_X;

        // M4: Bend top and bottom endpoints toward whitehole
        int16_t txD = applyGridDistortion(tx, HORIZON_Y, bhSX, bhSY, distortStrength, distortRadiusSq);
        int16_t bxD = applyGridDistortion(bx, SCREEN_H - 1, bhSX, bhSY, distortStrength, distortRadiusSq);

        renderer.drawLine(txD, HORIZON_Y, bxD, SCREEN_H - 1, COLOR_WHITE);
    }

    // 4. Quadratic foreshortened depth lines (scrolling with camY)
    //    M4: Drawn as segmented polylines with per-segment distortion toward whitehole
    int16_t camY_int = (int16_t)FP32_TO_INT(world.camY);
    int16_t zOffset = camY_int % Z_PERIOD;
    if (zOffset < 0) zOffset += Z_PERIOD;

    int16_t segW = SCREEN_W / GRID_HSEG_COUNT;

    for (int16_t z = Z_PERIOD - zOffset; z <= PERSPECTIVE_MAX_Z; z += Z_PERIOD) {
        int32_t z32 = z;
        int16_t yLine = HORIZON_Y + (int16_t)(((int32_t)GROUND_HEIGHT * z32 * z32) / PERSPECTIVE_MAX_Z_SQ);
        if (yLine <= 0 || yLine >= SCREEN_H) continue;

        // Draw distorted horizontal line as segmented polyline
        int16_t prevX = 0;
        int16_t prevY = yLine;

        for (uint8_t s = 1; s <= GRID_HSEG_COUNT; s++) {
            int16_t cx = (s == GRID_HSEG_COUNT) ? (SCREEN_W - 1) : (s * segW);
            int16_t cy = yLine;

            // Distort this control point toward whitehole
            int16_t dxBh = cx - bhSX;
            int16_t dyBh = cy - bhSY;
            int32_t d2 = (int32_t)dxBh * dxBh + (int32_t)dyBh * dyBh;

            if (d2 > 0 && d2 < (int32_t)distortRadiusSq) {
                // Linear falloff: stronger distortion closer to whitehole
                int32_t factor = (int32_t)distortRadiusSq - d2;
                int16_t shiftX = (int16_t)(((int32_t)(-dxBh) * distortStrength * factor) /
                                           ((int32_t)distortRadiusSq * 64));
                int16_t shiftY = (int16_t)(((int32_t)(-dyBh) * distortStrength * factor) /
                                           ((int32_t)distortRadiusSq * 64));
                cx += shiftX;
                cy += shiftY;
            }

            renderer.drawLine(prevX, prevY, cx, cy, COLOR_WHITE);
            prevX = cx;
            prevY = cy;
        }
    }
}

void Game::drawSpriteWithConfig(HalRenderer& renderer, int16_t x, int16_t y,
                                const uint8_t* bitmap, const uint8_t* mask,
                                const uint8_t* outline, uint8_t frame,
                                SpriteAlphaMode alphaMode,
                                SpriteOutlineMode outlineMode) {
    if (alphaMode == SPRITE_ALPHA_TRANSPARENT) {
        // Native transparent sprite: white pixels drawn, 0-bits untouched
        renderer.drawSelfMasked(x, y, bitmap, frame);
        return;
    }

    // Opaque background mode (solid black background behind sprite)
    switch (outlineMode) {
        case SPRITE_OUTLINE_BLACK:
            // 1. Erase dilated outline footprint to BLACK (erases background grid lines)
            if (outline != nullptr) {
                renderer.drawErase(x, y, outline, frame);
            } else if (mask != nullptr) {
                renderer.drawErase(x, y, mask, frame);
            }
            // 2. Draw white bitmap art pixels on top
            renderer.drawSelfMasked(x, y, bitmap, frame);
            break;

        case SPRITE_OUTLINE_WHITE:
            // 1. Draw dilated outline footprint in WHITE
            if (outline != nullptr) {
                renderer.drawSelfMasked(x, y, outline, frame);
            }
            // 2. Erase interior mask to BLACK and draw white bitmap art
            if (mask != nullptr) {
                renderer.drawExternalMask(x, y, bitmap, mask, frame, frame);
            } else {
                renderer.drawSelfMasked(x, y, bitmap, frame);
            }
            break;

        case SPRITE_OUTLINE_NONE:
        default:
            // Solid black interior background, no extra outline border
            if (mask != nullptr) {
                renderer.drawExternalMask(x, y, bitmap, mask, frame, frame);
            } else {
                renderer.drawSelfMasked(x, y, bitmap, frame);
            }
            break;
    }
}

void Game::renderEntities(HalRenderer& renderer) {
    for (uint8_t i = 0; i < MAX_ENTITIES; i++) {
        const Entity& e = entities.entities[i];
        if (!e.active) continue;

        // Project entity onto perspective ground grid
        int16_t sx = world.worldToScreenX(e.x, e.y);
        int16_t sy = world.worldToScreenY(e.y);

        // Skip if completely off-screen
        if (sx < -ITEM_SPRITE_WIDTH || sx > SCREEN_W + ITEM_SPRITE_WIDTH ||
            sy < -ITEM_SPRITE_HEIGHT || sy > SCREEN_H + ITEM_SPRITE_HEIGHT) {
            continue;
        }

        uint8_t frame = 0;
        switch (e.type) {
            case ENTITY_FOOD_APPLE:
                frame = SPRITE_ITEM_APPLE;
                break;
            case ENTITY_FOOD_PIZZA:
                frame = SPRITE_ITEM_PIZZA;
                break;
            case ENTITY_FOOD_TACO:
                frame = SPRITE_ITEM_TACO;
                break;
            case ENTITY_FOOD_BURGER:
                frame = SPRITE_ITEM_BURGER;
                break;
            case ENTITY_FOOD_FRIES:
                frame = SPRITE_ITEM_FRIES;
                break;
            case ENTITY_FOOD_CAKE:
                frame = SPRITE_ITEM_CAKE;
                break;
            case ENTITY_FOOD_DONUT:
                frame = SPRITE_ITEM_DONUT;
                break;
            case ENTITY_FOOD_ICECREAM:
                frame = SPRITE_ITEM_ICECREAM;
                break;
            case ENTITY_COLLECTIBLE_DIAMOND:
                frame = SPRITE_ITEM_DIAMOND;
                break;
            case ENTITY_COLLECTIBLE_BILLS:
                frame = SPRITE_ITEM_BILLS;
                break;
            case ENTITY_POWERUP_COFFEE:
                frame = SPRITE_ITEM_COFFEE;
                break;
            default:
                continue;
        }

        drawSpriteWithConfig(renderer,
                             sx - (ITEM_SPRITE_WIDTH / 2),
                             sy - (ITEM_SPRITE_HEIGHT / 2),
                             items_sprites, items_masks, items_outlines, frame,
                             ENTITY_ALPHA_MODE, ENTITY_OUTLINE_MODE);
    }
}

void Game::renderPlayer(HalRenderer& renderer) {
    // Player position on perspective ground plane
    int16_t sx = world.worldToScreenX(player.x, player.y);
    int16_t sy = world.worldToScreenY(player.y);

    // Speed boost visual effect: motion trail particles behind movement vector
    if (player.isBoosted() && (player.vx != 0 || player.vy != 0)) {
        int16_t trailX = sx - FP_TO_INT(player.vx * 3);
        int16_t trailY = sy - FP_TO_INT(player.vy * 3);
        if ((world.gameTime % 2) == 0) {
            renderer.drawPixel(trailX - 2, trailY, COLOR_WHITE);
            renderer.drawPixel(trailX + 2, trailY, COLOR_WHITE);
            renderer.drawPixel(trailX, trailY - 2, COLOR_WHITE);
        }
    }

    // Visual feedback for slow debuff: blink (slower cadence)
    if (!player.isSlowed() || ((player.slowTimer / PLAYER_BLINK_DIVISOR) % 2 == 0)) {
        uint8_t playerFrame = player.facingDir + (player.walkFrame * 8);
        drawSpriteWithConfig(renderer,
                             sx - (PLAYER_SPRITE_WIDTH / 2),
                             sy - (PLAYER_SPRITE_HEIGHT / 2),
                             player_sprite, player_mask, player_outline, playerFrame,
                             PLAYER_ALPHA_MODE, PLAYER_OUTLINE_MODE);
    }
}

void Game::renderBlackhole(HalRenderer& renderer) {
    // Project whitehole/blackhole onto perspective ground grid
    int16_t sx = world.worldToScreenX(world.bhX, world.bhY);
    int16_t sy = world.worldToScreenY(world.bhY);

    // Skip drawing if completely off-screen
    if (sx < -WHITEHOLE_SPRITE_WIDTH || sx > SCREEN_W + WHITEHOLE_SPRITE_WIDTH ||
        sy < -WHITEHOLE_SPRITE_HEIGHT || sy > SCREEN_H + WHITEHOLE_SPRITE_HEIGHT) {
        return;
    }

    // Animate swirling whitehole frames based on bhSpin (slower, majestic)
    uint8_t frame = (world.bhSpin / WHITEHOLE_ANIM_DIVISOR) % WHITEHOLE_FRAME_COUNT;
    drawSpriteWithConfig(renderer,
                         sx - (WHITEHOLE_SPRITE_WIDTH / 2),
                         sy - (WHITEHOLE_SPRITE_HEIGHT / 2),
                         whitehole_sprite, whitehole_mask, nullptr, frame,
                         WHITEHOLE_ALPHA_MODE, WHITEHOLE_OUTLINE_MODE);
}

void Game::renderHUD(HalRenderer& renderer) {
    // Configurable HUD position with black backing for readability over grid
    static const uint8_t HUD_PAD   = 2;    // Padding from screen edge
    static const uint8_t HUD_BG_W  = 42;   // Background width (fits score + combo multiplier)
    static const uint8_t HUD_BG_H  = 12;   // Background height (font 8px + 4px padding)

    int16_t hudX, hudY;
    switch (HUD_POSITION) {
        case HUD_TOP_LEFT:     hudX = HUD_PAD;                        hudY = HUD_PAD; break;
        case HUD_BOTTOM_LEFT:  hudX = HUD_PAD;                        hudY = SCREEN_H - HUD_BG_H - HUD_PAD + 1; break;
        case HUD_BOTTOM_RIGHT: hudX = SCREEN_W - HUD_BG_W - HUD_PAD;  hudY = SCREEN_H - HUD_BG_H - HUD_PAD + 1; break;
        case HUD_TOP_RIGHT:
        default:               hudX = SCREEN_W - HUD_BG_W - HUD_PAD;  hudY = HUD_PAD; break;
    }

    // Black background rectangle for readability over grid lines
    renderer.fillRect(hudX, hudY, HUD_BG_W, HUD_BG_H, COLOR_BLACK);
    // White border line outline
    renderer.drawRect(hudX, hudY, HUD_BG_W, HUD_BG_H, COLOR_WHITE);
    
    // Draw score text centered vertically within the box
    renderer.setCursor(hudX + 3, hudY + 2);
    renderer.printNumber(score);

    // Show combo multiplier or boost indicator
    if (comboMultiplier > 1) {
        renderer.print("x");
        renderer.printNumber(comboMultiplier);
    } else if (player.isBoosted()) {
        renderer.print(" !");
    }
}

// =============================================================================
// ACCRETION PARTICLE SYSTEM (M3)
// =============================================================================
// Lightweight particle system for the whitehole accretion disk visual effect.
// Particles spawn on the charge radius perimeter and spiral inward with orbital
// tangential drift. Each particle is a single white pixel with flicker effect.
//
// FOR FUTURE AGENTS / TUNING:
//   - PARTICLE_GRAVITY_MULT controls how fast particles spiral inward (3x default)
//   - PARTICLE_ORBITAL_SPEED controls tangential rotation speed
//   - PARTICLE_LIFETIME controls how long particles live before fading
//   - PARTICLE_SPAWN_INTERVAL controls spawn rate (lower = more particles visible)
//   - If particles look too sparse, reduce PARTICLE_SPAWN_INTERVAL or increase MAX_GRAVITY_PARTICLES
//   - If CPU is tight, reduce MAX_GRAVITY_PARTICLES or increase PARTICLE_SPAWN_INTERVAL

void Game::spawnParticle() {
    for (uint8_t i = 0; i < MAX_GRAVITY_PARTICLES; i++) {
        if (particles[i].life == 0) {
            // Spawn at random angle on the whitehole's charge radius perimeter
            uint8_t angle = (uint8_t)(world.nextRandom() & 255);
            int16_t chargePixels = FP_TO_INT(world.bhCharge);
            if (chargePixels < 20) chargePixels = 20;  // Minimum visual radius

            // Use 16-point unit circle lookup for position on perimeter
            uint8_t dir = (angle >> 4) & 15;  // Map 0-255 to 0-15 index
            int16_t r = chargePixels + (int16_t)(world.randomRange(0, 10));
            int16_t dx = (int16_t)(((int32_t)UNIT_CIRCLE_X[dir] * r) / 127);
            int16_t dy = (int16_t)(((int32_t)UNIT_CIRCLE_Y[dir] * r) / 127);

            particles[i].x = world.bhX + INT_TO_FP32(dx);
            particles[i].y = world.bhY + INT_TO_FP32(dy);
            particles[i].life = PARTICLE_LIFETIME;
            particles[i].angle = angle;
            return;
        }
    }
}

void Game::updateParticles(fp_t dt) {
    // Spawn new particles periodically
    particleSpawnTimer++;
    if (particleSpawnTimer >= PARTICLE_SPAWN_INTERVAL) {
        particleSpawnTimer = 0;
        spawnParticle();
    }

    // Update existing particles: pull toward whitehole center + slight orbital drift
    for (uint8_t i = 0; i < MAX_GRAVITY_PARTICLES; i++) {
        if (particles[i].life == 0) continue;

        // Strong inward pull (particles spiral inward faster than entities)
        applyBlackholeGravity(particles[i].x, particles[i].y,
                               world.bhX, world.bhY,
                               FP_MUL(world.bhMass, PARTICLE_GRAVITY_MULT), world.bhCharge, dt);

        // Orbital tangential drift (perpendicular to radial direction)
        fp32_t dx = particles[i].x - world.bhX;
        fp32_t dy = particles[i].y - world.bhY;
        // Tangent vector: (-dy, dx) normalized and scaled
        fp32_t dist = approxDistance(particles[i].x, particles[i].y, world.bhX, world.bhY);
        if (dist > FP_ONE) {
            fp32_t tangentX = (-dy * (fp32_t)PARTICLE_ORBITAL_SPEED) / dist;
            fp32_t tangentY = (dx * (fp32_t)PARTICLE_ORBITAL_SPEED) / dist;
            particles[i].x += FP32_MUL(tangentX, dt);
            particles[i].y += FP32_MUL(tangentY, dt);
        }

        // Decrement lifetime
        particles[i].life--;

        // Kill if very close to center (absorbed by whitehole core)
        if (dist < INT_TO_FP32(3)) {
            particles[i].life = 0;
        }
    }
}

void Game::renderParticles(HalRenderer& renderer) {
    for (uint8_t i = 0; i < MAX_GRAVITY_PARTICLES; i++) {
        if (particles[i].life == 0) continue;

        int16_t sx = world.worldToScreenX(particles[i].x, particles[i].y);
        int16_t sy = world.worldToScreenY(particles[i].y);

        // Skip if off-screen
        if (sx < 0 || sx >= SCREEN_W || sy < 0 || sy >= SCREEN_H) continue;

        // Flicker effect: draw every other frame for ethereal look
        if ((particles[i].life + i) % 2 == 0) {
            renderer.drawPixel(sx, sy, COLOR_WHITE);
        }
    }
}

// =============================================================================
// GRID DISTORTION HELPER (M4)
// =============================================================================
// Returns a distorted X coordinate for a grid point at (px, py), pulled toward
// the whitehole screen position (bhSX, bhSY) with linear falloff.
//
// FOR FUTURE AGENTS / TUNING:
//   - If distortion looks too strong, increase the divisor constant (64 below)
//   - If distortion looks too weak, decrease it or increase GRID_DISTORT_MAX_STRENGTH
//   - radiusSq should be GRID_DISTORT_RADIUS^2 (precomputed by caller)

int16_t Game::applyGridDistortion(int16_t px, int16_t py,
                                   int16_t bhSX, int16_t bhSY,
                                   int16_t strength, int16_t radiusSq) {
    int16_t dx = px - bhSX;
    int16_t dy = py - bhSY;
    int32_t d2 = (int32_t)dx * dx + (int32_t)dy * dy;

    if (d2 <= 0 || d2 >= (int32_t)radiusSq) {
        return px;  // Outside distortion radius — no effect
    }

    // Shift X toward whitehole, proportional to (radiusSq - d2) / radiusSq
    int32_t factor = (int32_t)radiusSq - d2;
    int16_t shiftX = (int16_t)(((int32_t)(-dx) * strength * factor) /
                               ((int32_t)radiusSq * 64));
    return px + shiftX;
}

// =============================================================================
// TITLE FONT HELPERS
// =============================================================================

uint8_t Game::getTitleCharWidth(char c) const {
    if (c < TITLE_FONT_ASCII_MIN || c > TITLE_FONT_ASCII_MAX) {
        return 4; // default space width
    }
    uint8_t idx = pgm_read_byte(&title_font_map[c - TITLE_FONT_ASCII_MIN]);
    if (idx == 255) return 4;
    const uint8_t* glyph_ptr = (const uint8_t*)&title_font_glyphs[idx];
    return pgm_read_byte(glyph_ptr);
}

uint16_t Game::getTitleTextWidth(const char* str) const {
    uint16_t w = 0;
    while (*str) {
        w += getTitleCharWidth(*str++);
        if (*str) w += 1; // 1px spacing between chars
    }
    return w;
}

void Game::drawTitleChar(HalRenderer& renderer, int16_t x, int16_t y, char c) {
    if (c < TITLE_FONT_ASCII_MIN || c > TITLE_FONT_ASCII_MAX) return;
    uint8_t idx = pgm_read_byte(&title_font_map[c - TITLE_FONT_ASCII_MIN]);
    if (idx == 255) return;

    const uint8_t* glyph_ptr = (const uint8_t*)&title_font_glyphs[idx];
    uint8_t width = pgm_read_byte(glyph_ptr);
    const uint8_t* cols = glyph_ptr + 1;

    for (uint8_t col = 0; col < width; col++) {
        uint8_t b = pgm_read_byte(cols + col);
        int16_t px = x + col;
        if (px < 0 || px >= SCREEN_W) continue;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (b & (1 << bit)) {
                int16_t py = y + bit;
                if (py >= 0 && py < SCREEN_H) {
                    renderer.drawPixel(px, py, COLOR_WHITE);
                }
            }
        }
    }
}

void Game::drawTitleText(HalRenderer& renderer, int16_t x, int16_t y, const char* str) {
    int16_t cur_x = x;
    while (*str) {
        char c = *str++;
        uint8_t w = getTitleCharWidth(c);
        drawTitleChar(renderer, cur_x, y, c);
        cur_x += w + 1; // 1px spacing
    }
}
