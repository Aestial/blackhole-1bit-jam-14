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
void Game::update(HalInput& input) {
    switch (state) {
        case STATE_TITLE:
            updateTitle(input);
            break;
        case STATE_PLAYING:
            updatePlaying(input);
            break;
        case STATE_GAMEOVER:
            updateGameOver(input);
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

void Game::updateTitle(HalInput& input) {
    if (input.justPressed(BTN_A)) {
        reset();
        state = STATE_PLAYING;
    }
}

void Game::renderTitle(HalRenderer& renderer) {
    // TODO(M4): Fancy title screen with blackhole animation
    // For now: simple text display

    // "SUPERMASSIVE" — 12 chars × 6px = 72px wide → x = (128-72)/2 = 28
    renderer.setCursor(28, 8);
    renderer.print("SUPERMASSIVE");

    // "BLACKHOLE" — 9 chars × 6px = 54px wide → x = (128-54)/2 = 37
    renderer.setCursor(37, 20);
    renderer.print("BLACKHOLE");

    // High score
    renderer.setCursor(28, 36);
    renderer.print("High:");
    renderer.printNumber(highScore);

    // "Press A" — 7 chars × 6px = 42px → x = (128-42)/2 = 43
    renderer.setCursor(43, 52);
    renderer.print("Press A");
}

// =============================================================================
// STATE: PLAYING
// =============================================================================

void Game::updatePlaying(HalInput& input) {
    // 1. Read input
    bool up    = input.pressed(BTN_UP);
    bool down  = input.pressed(BTN_DOWN);
    bool left  = input.pressed(BTN_LEFT);
    bool right = input.pressed(BTN_RIGHT);
    bool accel = input.pressed(BTN_A);
    bool brake = input.pressed(BTN_B);

    // 2. Update player movement
    player.update(up, down, left, right, accel, brake);

    // 3. Update world (camera, blackhole, difficulty, timers)
    world.update(player.x, player.y);

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

void Game::updateGameOver(HalInput& input) {
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
    // TODO(M1): Basic scrolling grid
    // TODO(M4): Converging grid toward blackhole
    //
    // BASIC GRID ALGORITHM (M1):
    //   // Vertical lines
    //   int16_t offsetX = (int16_t)(FP32_TO_INT(world.camX) % GRID_SPACING);
    //   for (int16_t x = -offsetX; x < SCREEN_W; x += GRID_SPACING) {
    //     renderer.drawLine(x, 0, x, SCREEN_H - 1, COLOR_WHITE);
    //   }
    //   // Horizontal lines
    //   int16_t offsetY = (int16_t)(FP32_TO_INT(world.camY) % GRID_SPACING);
    //   for (int16_t y = -offsetY; y < SCREEN_H; y += GRID_SPACING) {
    //     renderer.drawLine(0, y, SCREEN_W - 1, y, COLOR_WHITE);
    //   }
    //
    // CONVERGING GRID (M4):
    //   See game.h renderBackground() docs for the displacement algorithm.
    //   Displace grid intersections toward blackhole screen position.

    // STUB: Draw a simple dot pattern for now (very lightweight)
    int16_t offsetX = (int16_t)(FP32_TO_INT(world.camX) % GRID_SPACING);
    int16_t offsetY = (int16_t)(FP32_TO_INT(world.camY) % GRID_SPACING);

    // Handle negative modulo
    if (offsetX < 0) offsetX += GRID_SPACING;
    if (offsetY < 0) offsetY += GRID_SPACING;

    for (int16_t x = -offsetX; x <= SCREEN_W; x += GRID_SPACING) {
        for (int16_t y = -offsetY; y <= SCREEN_H; y += GRID_SPACING) {
            if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
                renderer.drawPixel(x, y, COLOR_WHITE);
            }
        }
    }
}

void Game::renderEntities(HalRenderer& renderer) {
    // TODO(M2): Draw each entity based on its type
    //
    // for (uint8_t i = 0; i < MAX_ENTITIES; i++) {
    //   Entity& e = entities.entities[i];
    //   if (!e.active) continue;
    //
    //   int16_t sx = world.worldToScreenX(e.x);
    //   int16_t sy = world.worldToScreenY(e.y);
    //
    //   // Skip if off-screen (optional optimization)
    //   if (sx < -8 || sx > SCREEN_W + 8 || sy < -8 || sy > SCREEN_H + 8) continue;
    //
    //   switch (e.type) {
    //     case ENTITY_FOOD_PIZZA:
    //       // Filled triangle (equilateral, tip pointing up)
    //       renderer.fillTriangle(sx, sy - 3,         // top vertex
    //                              sx - 3, sy + 3,    // bottom-left
    //                              sx + 3, sy + 3,    // bottom-right
    //                              COLOR_WHITE);
    //       break;
    //     case ENTITY_FOOD_BURGER:
    //       // Filled square
    //       renderer.fillRect(sx - 3, sy - 3, 6, 6, COLOR_WHITE);
    //       break;
    //     case ENTITY_FOOD_DONUT:
    //       // Filled circle
    //       renderer.fillCircle(sx, sy, DONUT_SPRITE_RADIUS, COLOR_WHITE);
    //       break;
    //     case ENTITY_COLLECTIBLE:
    //       // Diamond (rotated square) — draw as 4 lines
    //       renderer.drawLine(sx, sy - 2, sx + 2, sy, COLOR_WHITE); // top-right
    //       renderer.drawLine(sx + 2, sy, sx, sy + 2, COLOR_WHITE); // right-bottom
    //       renderer.drawLine(sx, sy + 2, sx - 2, sy, COLOR_WHITE); // bottom-left
    //       renderer.drawLine(sx - 2, sy, sx, sy - 2, COLOR_WHITE); // left-top
    //       break;
    //   }
    // }
}

void Game::renderPlayer(HalRenderer& renderer) {
    // Player is always at screen center (camera follows directly)
    int16_t sx = SCREEN_W / 2;
    int16_t sy = SCREEN_H / 2;

    // Draw fat man as filled circle
    renderer.fillCircle(sx, sy, PLAYER_SPRITE_RADIUS, COLOR_WHITE);

    // TODO(M1): Visual feedback for food slow debuff
    //   if (player.isSlowed()) {
    //     // Blink: only draw on even frames, or draw a smaller circle
    //     // Or draw an outline ring to indicate debuff
    //     renderer.drawCircle(sx, sy, PLAYER_SPRITE_RADIUS + 2, COLOR_WHITE);
    //   }

    // TODO(M5): Replace with directional sprite based on player.desiredDx/Dy
    //   Choose from 8 sprite frames based on facing direction
}

void Game::renderBlackhole(HalRenderer& renderer) {
    int16_t sx = world.worldToScreenX(world.bhX);
    int16_t sy = world.worldToScreenY(world.bhY);

    // TODO(M3): Fancy blackhole with concentric rings and spin animation
    //
    // Concentric circles (gravity well effect):
    //   renderer.fillCircle(sx, sy, BH_RENDER_RADIUS, COLOR_WHITE);
    //   renderer.fillCircle(sx, sy, BH_RENDER_RADIUS - 2, COLOR_BLACK);
    //   renderer.drawCircle(sx, sy, BH_RENDER_RADIUS - 4, COLOR_WHITE);
    //   renderer.fillCircle(sx, sy, 2, COLOR_BLACK);
    //
    // Spin animation (cosmetic):
    //   Use world.bhSpin to rotate radiating lines:
    //   for each of 4 "arms" at angle (bhSpin * 2 + arm * 64):
    //     Draw a short line from center outward
    //     (Use lookup table for sin/cos if available, or just 4 fixed offsets)

    // STUB: Simple circle for now
    renderer.drawCircle(sx, sy, BH_RENDER_RADIUS, COLOR_WHITE);
    renderer.drawCircle(sx, sy, BH_RENDER_RADIUS / 2, COLOR_WHITE);
    renderer.drawPixel(sx, sy, COLOR_WHITE);
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
