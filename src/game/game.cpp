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
    // TODO(M2): Implement entity spawning
    //   if (world.shouldSpawn()) {
    //     // Choose random type: 60% food (equal split), 40% collectible
    //     // Choose random position: SPAWN_MIN_DISTANCE to SPAWN_RADIUS from player
    //     // entities.spawn(type, worldX, worldY);
    //     world.resetSpawnTimer();
    //   }

    // 5. Apply blackhole gravity to entities
    // TODO(M3): Uncomment when ready
    //   for (uint8_t i = 0; i < MAX_ENTITIES; i++) {
    //     if (entities.entities[i].active) {
    //       applyBlackholeGravity(entities.entities[i].x, entities.entities[i].y,
    //                              world.bhX, world.bhY,
    //                              world.bhMass, world.bhCharge);
    //     }
    //   }

    // 6. Check player-entity collisions
    // TODO(M2): Implement collision responses
    //   for (uint8_t i = 0; i < MAX_ENTITIES; i++) {
    //     if (!entities.entities[i].active) continue;
    //     Entity& e = entities.entities[i];
    //     if (checkOverlap(player.x, player.y, player.width, player.height,
    //                       e.x, e.y, e.width, e.height)) {
    //       if (e.isFood()) {
    //         player.applyFoodSlow(e.type);
    //       } else if (e.isCollectible()) {
    //         score += SCORE_PER_COLLECTIBLE;
    //       }
    //       entities.despawn(i);
    //     }
    //   }

    // 7. Despawn far entities
    // TODO(M2): Implement despawning
    //   for (uint8_t i = 0; i < MAX_ENTITIES; i++) {
    //     if (entities.entities[i].active && world.isTooFar(entities.entities[i].x, entities.entities[i].y)) {
    //       entities.despawn(i);
    //     }
    //   }

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
    // Milestone 1 (M1): Clean Pseudo-3D Perspective Ground Grid
    // =========================================================================
    // Renders an infinite perspective grid with a clean horizon at y = 14:
    // - Above horizon (y < 14): Open celestial void (clean space for HUD/score).
    // - Horizon line drawn at y = 14.
    // - Perspective rays radiate outward from horizon to screen bottom, scrolling with camX.
    // - Horizontal depth lines are geometrically foreshortened (quadratic scale),
    //   scrolling smoothly with camY.

    // 1. Horizon Line
    renderer.drawLine(0, HORIZON_Y, SCREEN_W - 1, HORIZON_Y, COLOR_WHITE);

    // 2. Perspective Rays (scrolling with camX)
    int16_t camX_int = (int16_t)FP32_TO_INT(world.camX);
    int16_t xOffset = camX_int % BASE_SPACING_X;
    if (xOffset < 0) xOffset += BASE_SPACING_X;

    for (int16_t bx = -xOffset - BASE_SPACING_X * 2; bx <= SCREEN_W + BASE_SPACING_X * 2; bx += BASE_SPACING_X) {
        int16_t tx = (SCREEN_W / 2) + ((bx - (SCREEN_W / 2)) * TOP_SPACING_X) / BASE_SPACING_X;
        renderer.drawLine(tx, HORIZON_Y, bx, SCREEN_H - 1, COLOR_WHITE);
    }

    // 3. Geometrically foreshortened depth lines (scrolling smoothly with camY)
    int16_t camY_int = (int16_t)FP32_TO_INT(world.camY);
    int16_t zOffset = camY_int % Z_PERIOD;
    if (zOffset < 0) zOffset += Z_PERIOD;

    // Ground height: 64 - 14 = 50px. Quadratic curve: y = HORIZON_Y + (50 * z^2) / 14400
    for (int16_t z = Z_PERIOD - zOffset; z <= 120; z += Z_PERIOD) {
        int32_t z32 = z;
        int16_t yLine = HORIZON_Y + (int16_t)((50 * z32 * z32) / 14400);
        if (yLine > HORIZON_Y && yLine < SCREEN_H) {
            renderer.drawLine(0, yLine, SCREEN_W - 1, yLine, COLOR_WHITE);
        }
    }
}

void Game::renderEntities(HalRenderer& renderer) {
    for (uint8_t i = 0; i < MAX_ENTITIES; i++) {
        const Entity& e = entities.entities[i];
        if (!e.active) continue;

        // Project entity onto perspective ground grid
        int16_t sx = world.worldToScreenX(e.x, e.y);
        int16_t sy = world.worldToScreenY(e.y);

        // Skip if completely off-screen or far beyond horizon
        if (sx < -ITEM_SPRITE_WIDTH || sx > SCREEN_W + ITEM_SPRITE_WIDTH ||
            sy < HORIZON_Y - ITEM_SPRITE_HEIGHT || sy > SCREEN_H + ITEM_SPRITE_HEIGHT) {
            continue;
        }

        uint8_t frame = 0;
        switch (e.type) {
            case ENTITY_FOOD_PIZZA:
                frame = SPRITE_ITEM_PIZZA;
                break;
            case ENTITY_FOOD_BURGER:
                frame = SPRITE_ITEM_BURGER;
                break;
            case ENTITY_FOOD_DONUT:
                frame = SPRITE_ITEM_DONUT;
                break;
            case ENTITY_FOOD_ICECREAM:
                frame = SPRITE_ITEM_ICECREAM;
                break;
            case ENTITY_COLLECTIBLE:
                frame = SPRITE_ITEM_COLLECTIBLE;
                break;
            default:
                continue;
        }

        renderer.drawSelfMasked(sx - (ITEM_SPRITE_WIDTH / 2),
                                sy - (ITEM_SPRITE_HEIGHT / 2),
                                items_sprites, frame);
    }
}

void Game::renderPlayer(HalRenderer& renderer) {
    // Player position on perspective ground plane
    int16_t sx = world.worldToScreenX(player.x, player.y);
    int16_t sy = world.worldToScreenY(player.y);

    // Visual feedback for slow debuff: blink (slower cadence)
    if (!player.isSlowed() || ((player.slowTimer / PLAYER_BLINK_DIVISOR) % 2 == 0)) {
        renderer.drawSelfMasked(sx - (PLAYER_SPRITE_WIDTH / 2),
                                sy - (PLAYER_SPRITE_HEIGHT / 2),
                                player_sprite, 0);
    }
}

void Game::renderBlackhole(HalRenderer& renderer) {
    // Project whitehole/blackhole onto perspective ground grid
    int16_t sx = world.worldToScreenX(world.bhX, world.bhY);
    int16_t sy = world.worldToScreenY(world.bhY);

    // Skip drawing if completely off-screen or far beyond horizon
    if (sx < -WHITEHOLE_SPRITE_WIDTH || sx > SCREEN_W + WHITEHOLE_SPRITE_WIDTH ||
        sy < HORIZON_Y - WHITEHOLE_SPRITE_HEIGHT || sy > SCREEN_H + WHITEHOLE_SPRITE_HEIGHT) {
        return;
    }

    // Animate swirling whitehole frames based on bhSpin (slower, majestic)
    uint8_t frame = (world.bhSpin / WHITEHOLE_ANIM_DIVISOR) % WHITEHOLE_FRAME_COUNT;
    renderer.drawSelfMasked(sx - (WHITEHOLE_SPRITE_WIDTH / 2),
                            sy - (WHITEHOLE_SPRITE_HEIGHT / 2),
                            whitehole_sprite, frame);
}

void Game::renderHUD(HalRenderer& renderer) {
    // TODO(M4): Clean HUD with background bar for readability
    //
    // Score in top-right:
    //   renderer.fillRect(SCREEN_W - 42, 0, 42, 9, COLOR_BLACK); // background
    //   renderer.setCursor(SCREEN_W - 40, 1);
    //   renderer.printNumber(score);

    // STUB: Score in top-right, no background
    renderer.setCursor(SCREEN_W - 36, 0);
    renderer.printNumber(score);
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
