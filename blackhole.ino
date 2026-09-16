// =============================================================================
// blackhole.ino — Arduino Entry Point for Supermassive Blackhole
// =============================================================================
//
// PURPOSE:
//   This is the Arduino sketch entry point. It:
//   1. Creates the Arduboy2 hardware instance
//   2. Creates HAL wrappers (renderer, input, storage)
//   3. Creates the Game instance
//   4. Wires them together in setup() and runs the game loop in loop()
//
// GAME LOOP:
//   Every frame (60 FPS target):
//     1. Check if it's time for a new frame (Arduboy2::nextFrame)
//     2. Poll button inputs
//     3. Update game logic (state machine, physics, spawning, collisions)
//     4. Clear the screen buffer
//     5. Render the current game state
//     6. Push the buffer to the OLED display
//
// FOR IMPLEMENTING AGENTS:
//   This file should rarely need changes after M0. The only reasons to
//   modify it would be:
//   - Adding new HAL instances (e.g., audio in M6)
//   - Changing frame rate
//   - Adding debug output
//
// =============================================================================

#include <Arduboy2.h>
#include "src/platform/arduboy/arduboy_renderer.h"
#include "src/platform/arduboy/arduboy_input.h"
#include "src/platform/arduboy/arduboy_storage.h"
#include "src/game/game.h"

// =============================================================================
// Global instances
// =============================================================================
// These are created once and live for the entire program lifetime.
// No dynamic allocation — everything is static.

Arduboy2 arduboy;                       // Hardware abstraction (Arduboy2 library)
ArduboyRenderer renderer(arduboy);      // Renderer HAL → wraps Arduboy2 draw calls
ArduboyInput input(arduboy);            // Input HAL → wraps Arduboy2 button calls
ArduboyStorage storage;                 // Storage HAL → wraps EEPROM
Game game;                              // The game itself

// =============================================================================
// setup() — Called once at power-on / reset
// =============================================================================
void setup() {
    arduboy.begin();
    arduboy.setFrameRate(TARGET_FPS);
    game.init(storage);
}

// =============================================================================
// loop() — Called repeatedly (main game loop)
// =============================================================================
void loop() {
    // Wait until it's time for the next frame (60 FPS timing)
    if (!arduboy.nextFrame()) return;

    // 1. Poll button state (must be before any input checks)
    input.poll();

    // 2. Update game logic
    game.update(input);

    // 3. Render
    renderer.clear();
    game.render(renderer);
    renderer.display();
}
