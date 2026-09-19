// =============================================================================
// game.h — Game State Machine & Rendering Bridge
// =============================================================================
//
// PURPOSE:
//   The central orchestrator. Game owns all game data (Player, World,
//   EntityManager) and is the ONLY game logic file that touches HAL types.
//   It implements:
//   - State machine (Title → Playing → GameOver)
//   - Per-frame update dispatch (input → physics → spawning → collision)
//   - ALL rendering (background, entities, player, blackhole, HUD, menus)
//
// ARCHITECTURE:
//   This is the BRIDGE between platform-independent game data and the HAL.
//   It includes hal_types.h to get the concrete renderer/input/storage types.
//   All other game files (player.h, entity.h, world.h, physics.h) are pure
//   C++ and never see HAL types.
//
//   Data flow per frame:
//   ┌─────────────────────────────────────────────────────────┐
//   │ .ino: input.poll() → game.update(input) → game.render(renderer) │
//   └─────────────────────────────────────────────────────────┘
//
//   Inside game.update():
//   ┌─────────────────────────────────────────────────────────────────────┐
//   │ read input → player.update() → world.update() → physics checks    │
//   │ → spawn entities → despawn far entities → check game over          │
//   └─────────────────────────────────────────────────────────────────────┘
//
//   Inside game.render():
//   ┌─────────────────────────────────────────────────────────────────────┐
//   │ renderBackground() → renderEntities() → renderPlayer()            │
//   │ → renderBlackhole() → renderHUD()                                  │
//   └─────────────────────────────────────────────────────────────────────┘
//
// GAME STATES:
//   STATE_TITLE    — Show title, high score, "Press A to Start"
//   STATE_PLAYING  — Main gameplay loop
//   STATE_GAMEOVER — Show final score, high score, retry/menu prompts
//
// FOR IMPLEMENTING AGENTS:
//   - M1: Fill in renderPlayer(), renderBackground() (basic grid)
//   - M2: Fill in updatePlaying() (spawning, collision, scoring)
//         Fill in renderEntities() (draw each entity type as placeholder shape)
//   - M3: Fill in renderBlackhole(), enable gravity in updatePlaying()
//   - M4: Fill in renderTitle(), renderGameOver(), renderHUD()
//         Fill in renderBackground() with converging grid effect
//   - Each render method receives a HalRenderer& and calls draw methods
//     (fillCircle, fillRect, drawLine, etc.) as documented in hal/renderer.h
//
// =============================================================================

#ifndef GAME_H
#define GAME_H

#include "config.h"
#include "player.h"
#include "entity.h"
#include "world.h"
#include "../hal/hal_types.h"  // Brings in HalRenderer, HalInput, HalStorage

// =============================================================================
// GameState — Which screen/mode the game is in
// =============================================================================
enum GameState : uint8_t {
    STATE_TITLE    = 0,  // Title screen — waiting for player to start
    STATE_PLAYING  = 1,  // Main gameplay — running, dodging, collecting
    STATE_GAMEOVER = 2   // Game over — showing score, waiting for input
};

// =============================================================================
// Game — The central game class
// =============================================================================
// Size: Player (~30) + World (~50) + EntityManager (~168) + misc (~10) ≈ 258 bytes
// One instance, created globally in supermassive-whitehole.ino.

class Game {
public:
    // ---- Game data (public for debug access; can be made private later) ----
    Player player;
    World world;
    EntityManager entities;
    GameState state;
    uint16_t score;
    uint16_t highScore;
    uint8_t  comboMultiplier;
    uint8_t  gameOverCooldown; // Frames remaining before accepting input on game-over screen

    // =========================================================================
    // init(storage) — One-time initialization
    // =========================================================================
    // Called once in setup(). Loads high score from persistent storage
    // and enters the title state.
    //
    // Parameters:
    //   storage — Platform storage for loading high score
    void init(HalStorage& storage);

    // =========================================================================
    // update(input) — Per-frame game logic update
    // =========================================================================
    // Called once per frame. Dispatches to the appropriate state handler.
    // This is where ALL game logic happens — input reading, physics,
    // spawning, collision detection, scoring, game over check.
    //
    // Parameters:
    //   input — Platform input for reading button states
    //
    // State dispatch:
    //   STATE_TITLE:    wait for A press → start game
    //   STATE_PLAYING:  full gameplay update (see updatePlaying)
    //   STATE_GAMEOVER: wait for A (title) or B (retry)
    void update(HalInput& input, fp_t dt = FP_DT_ONE);

    // =========================================================================
    // render(renderer) — Per-frame rendering
    // =========================================================================
    // Called once per frame after update(). Draws the current state to screen.
    // The renderer has already been cleared by the .ino before this call.
    //
    // Parameters:
    //   renderer — Platform renderer for drawing
    //
    // State dispatch:
    //   STATE_TITLE:    renderTitle()
    //   STATE_PLAYING:  renderPlaying()
    //   STATE_GAMEOVER: renderGameOver()
    void render(HalRenderer& renderer);

private:
    HalStorage* storageRef;  // Pointer to storage (for saving high score at game over)

    // =========================================================================
    // reset() — Reset game state for a new game
    // =========================================================================
    // Called when transitioning from TITLE or GAMEOVER to PLAYING.
    // Resets player, world, entity manager, and score. Does NOT reset highScore.
    void reset();

    // ---- State-specific update handlers ----

    // =========================================================================
    // updateTitle(input) — Title screen update
    // =========================================================================
    // Checks for A button press to start the game.
    // On A press: reset() → state = STATE_PLAYING
    void updateTitle(HalInput& input, fp_t dt = FP_DT_ONE);

    // =========================================================================
    // updatePlaying(input) — Main gameplay update
    // =========================================================================
    // The big one. Executes this sequence every frame:
    //
    //   1. READ INPUT:
    //      bool up    = input.pressed(BTN_UP);
    //      bool down  = input.pressed(BTN_DOWN);
    //      ... (all 6 buttons)
    //
    //   2. UPDATE PLAYER:
    //      player.update(up, down, left, right, accel, brake);
    //
    //   3. UPDATE WORLD:
    //      world.update(player.x, player.y);
    //      (moves blackhole, ramps difficulty, decrements timers)
    //
    //   4. SPAWN ENTITIES (M2):
    //      if (world.shouldSpawn()):
    //        Choose random EntityType (weighted: 60% food, 40% collectible)
    //        Choose random position within SPAWN_RADIUS of player,
    //          at least SPAWN_MIN_DISTANCE away
    //        entities.spawn(type, worldX, worldY)
    //        world.resetSpawnTimer()
    //
    //   5. APPLY BLACKHOLE GRAVITY TO ENTITIES (M3):
    //      for each active entity:
    //        applyBlackholeGravity(entity.x, entity.y,
    //                              world.bhX, world.bhY,
    //                              world.bhMass, world.bhCharge)
    //
    //   6. CHECK PLAYER-ENTITY COLLISIONS (M2):
    //      for each active entity:
    //        if checkOverlap(player, entity):
    //          if entity.isFood():    player.applyFoodSlow(entity.type)
    //          if entity.isCollectible(): score += SCORE_PER_COLLECTIBLE
    //          entities.despawn(i)
    //
    //   7. DESPAWN FAR ENTITIES (M2):
    //      for each active entity:
    //        if world.isTooFar(entity.x, entity.y):
    //          entities.despawn(i)
    //
    //   8. PASSIVE SCORE (M2):
    //      if world.shouldScoreTick():
    //        score += SCORE_PER_TICK
    //        world.resetScoreTimer()
    //
    //   9. CHECK GAME OVER (M3):
    //      if checkOverlap(player, blackhole):
    //        state = STATE_GAMEOVER
    //        if (score > highScore): highScore = score; storageRef->saveHighScore(score)
    //
    void updatePlaying(HalInput& input, fp_t dt = FP_DT_ONE);

    // =========================================================================
    // updateGameOver(input, dt) — Game over screen update
    // =========================================================================
    // Checks for button presses:
    //   A button: go back to title (state = STATE_TITLE)
    //   B button: retry immediately (reset() + state = STATE_PLAYING)
    void updateGameOver(HalInput& input, fp_t dt = FP_DT_ONE);

    // ---- State-specific render handlers ----

    // =========================================================================
    // renderTitle(renderer) — Draw the title screen
    // =========================================================================
    // Layout (128x64 screen):
    //   Line 1 (y=8):  "SUPERMASSIVE"  (centered)
    //   Line 2 (y=20): "BLACKHOLE"     (centered)
    //   Line 3 (y=36): "High: XXXXX"   (centered, show highScore)
    //   Line 4 (y=52): "Press A"       (centered, maybe blinking)
    //
    // FOR IMPLEMENTING AGENTS (M4):
    //   Use renderer.setCursor() + renderer.print() for text.
    //   Arduboy2 font is 6x8 pixels. To center text of N chars:
    //     x = (SCREEN_W - N * 6) / 2
    //   Optional: draw a placeholder blackhole animation using bhSpin
    void renderTitle(HalRenderer& renderer);

    // =========================================================================
    // renderPlaying(renderer) — Draw the main gameplay screen
    // =========================================================================
    // Draw order (back to front — painter's algorithm):
    //   1. renderBackground()  — Grid lines (behind everything)
    //   2. renderEntities()    — Food + collectibles
    //   3. renderBlackhole()   — The pursuing blackhole
    //   4. renderPlayer()      — The fat man (on top of most things)
    //   5. renderHUD()         — Score overlay (topmost)
    void renderPlaying(HalRenderer& renderer);

    // =========================================================================
    // renderGameOver(renderer) — Draw the game over screen
    // =========================================================================
    // Layout:
    //   Line 1 (y=8):  "GAME OVER"     (centered)
    //   Line 2 (y=24): "Score: XXXXX"  (centered)
    //   Line 3 (y=36): "High: XXXXX"   (centered)
    //   Line 4 (y=48): "A:Menu B:Retry" (centered)
    //
    // If new high score was set, maybe flash "NEW HIGH SCORE!" text.
    void renderGameOver(HalRenderer& renderer);

    // ---- Individual render components ----

    // =========================================================================
    // renderBackground(renderer) — Draw the infinite perspective ground grid
    // =========================================================================
    // Renders a pseudo-3D perspective plane with horizon at y=14:
    // - Horizon line separates open sky / celestial void from the ground plane.
    // - Perspective rays radiate outward to screen bottom, scrolling with camX.
    // - Foreshortened horizontal depth lines scroll with camY with quadratic scale.
    //
    // ADVANCED EXTENSION (M4 — converging toward blackhole):
    //   Optional space-time distortion around blackhole position.
    void renderBackground(HalRenderer& renderer);

    // =========================================================================
    // renderEntities(renderer) — Draw all active entities
    // =========================================================================
    // Iterate entityManager.entities[]. For each active entity:
    //   1. Convert world position to screen: sx, sy = worldToScreen()
    //   2. Skip if off-screen (optional optimization)
    //   3. Draw based on entity type:
    //      - FOOD_PIZZA:  fillTriangle (equilateral, centered on sx,sy)
    //      - FOOD_BURGER: fillRect (centered on sx,sy, FOOD_BURGER_SIZE square)
    //      - FOOD_DONUT:  fillCircle (sx, sy, DONUT_SPRITE_RADIUS)
    //      - COLLECTIBLE: draw diamond shape (4 lines or 2 fillTriangles)
    //   All drawn in COLOR_WHITE.
    void renderEntities(HalRenderer& renderer);

    // =========================================================================
    // renderPlayer(renderer) — Draw the player (fat man placeholder)
    // =========================================================================
    // The player is always at screen center (SCREEN_W/2, SCREEN_H/2) since
    // the camera tracks the player directly.
    //
    // Draw: fillCircle(SCREEN_W/2, SCREEN_H/2, PLAYER_SPRITE_RADIUS, COLOR_WHITE)
    //
    // If player is slowed (player.isSlowed()), maybe blink or draw smaller
    // to indicate the debuff visually.
    //
    // Future (M5): Replace with directional sprite based on player.desiredDx/Dy
    void renderPlayer(HalRenderer& renderer);

    // =========================================================================
    // renderBlackhole(renderer) — Draw the blackhole
    // =========================================================================
    // Convert blackhole world position to screen:
    //   sx = world.worldToScreenX(world.bhX)
    //   sy = world.worldToScreenY(world.bhY)
    //
    // Draw concentric circles to create a "gravity well" look:
    //   fillCircle(sx, sy, BH_RENDER_RADIUS, COLOR_WHITE)        // outer ring
    //   fillCircle(sx, sy, BH_RENDER_RADIUS - 2, COLOR_BLACK)   // inner gap
    //   drawCircle(sx, sy, BH_RENDER_RADIUS - 4, COLOR_WHITE)   // inner ring
    //   fillCircle(sx, sy, 2, COLOR_BLACK)                       // center void
    //
    // Use world.bhSpin to animate (e.g., offset rings by sin(bhSpin)):
    //   This is cosmetic — any subtle animation works.
    //
    // Optional: draw radiating lines from center to suggest spin/charge
    void renderBlackhole(HalRenderer& renderer);

    // =========================================================================
    // renderHUD(renderer) — Draw the heads-up display overlay
    // =========================================================================
    // Minimal HUD — just the score in the top-right corner:
    //   renderer.setCursor(SCREEN_W - 36, 0)  // Top-right, ~6 digits
    //   renderer.printNumber(score)
    //
    // Optional: small icon or bar indicating food slow status
    // Optional: distance indicator to blackhole
    void renderHUD(HalRenderer& renderer);

    // ---- Title screen font rendering helpers ----
    void drawTitleChar(HalRenderer& renderer, int16_t x, int16_t y, char c);
    void drawTitleText(HalRenderer& renderer, int16_t x, int16_t y, const char* str);
    uint8_t getTitleCharWidth(char c) const;
    uint16_t getTitleTextWidth(const char* str) const;

    // ---- Sprite rendering helper with transparency and outline control ----
    void drawSpriteWithConfig(HalRenderer& renderer, int16_t x, int16_t y,
                              const uint8_t* bitmap, const uint8_t* mask,
                              const uint8_t* outline, uint8_t frame,
                              SpriteAlphaMode alphaMode,
                              SpriteOutlineMode outlineMode);

    // =========================================================================
    // Whitehole Accretion Particles (M3)
    // =========================================================================
    // Lightweight particle system for visual accretion disk effect around the
    // whitehole. Particles spawn on the charge radius perimeter and spiral
    // inward with orbital tangential drift. Each is a single flickering pixel.
    //
    // RAM cost: MAX_GRAVITY_PARTICLES * 10 bytes + 1 byte timer = 81 bytes
    //
    // FOR IMPLEMENTING AGENTS:
    //   - spawnParticle(): Find first dead slot, place on charge perimeter
    //   - updateParticles(): Apply strong inward gravity + orbital drift
    //   - renderParticles(): Draw single pixels with flicker effect
    //   - Called from updatePlaying() and renderPlaying() respectively

    struct GravityParticle {
        fp32_t x;       // World-space X (Q24.8)
        fp32_t y;       // World-space Y (Q24.8)
        uint8_t life;   // Frames remaining (0 = inactive)
        uint8_t angle;  // Current orbital angle (0-255, for respawn positioning)
    };

    GravityParticle particles[MAX_GRAVITY_PARTICLES];
    uint8_t particleSpawnTimer;

    void spawnParticle();
    void updateParticles(fp_t dt);
    void renderParticles(HalRenderer& renderer);

    // =========================================================================
    // Grid Distortion Helper (M4)
    // =========================================================================
    // Returns a distorted X coordinate for a grid point at (px, py),
    // pulled toward the whitehole screen position (bhSX, bhSY).
    // Uses linear falloff within radiusSq. Returns px unchanged if outside range.
    //
    // FOR IMPLEMENTING AGENTS:
    //   - Called per ray endpoint in renderBackground()
    //   - Pure function (no side effects), can be static
    static int16_t applyGridDistortion(int16_t px, int16_t py,
                                        int16_t bhSX, int16_t bhSY,
                                        int16_t strength, int16_t radiusSq);
};

#endif // GAME_H
