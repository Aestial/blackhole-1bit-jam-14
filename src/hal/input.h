// =============================================================================
// input.h — Input HAL Interface Contract
// =============================================================================
//
// PURPOSE:
//   Defines the input API and button enumeration that any platform
//   implementation MUST provide.
//
// ARDUBOY HARDWARE:
//   6 buttons: D-pad (UP/DOWN/LEFT/RIGHT) + A + B
//   No analog sticks, no triggers, no touch. This is our full input space.
//
// BUTTON SEMANTICS IN THIS GAME:
//   UP/DOWN/LEFT/RIGHT — Set the player's desired movement direction.
//                        Combinations set diagonals (e.g., UP+RIGHT).
//   A — Accelerate (apply thrust in desired direction)
//   B — Brake (increase friction, decelerate)
//
//   In menus:
//   A — Confirm / Start
//   B — Retry / Back
//
// FOR PLATFORM IMPLEMENTERS:
//   Implement a struct with these exact methods. Map your platform's
//   button/key constants to the Button enum values.
//   See: src/platform/arduboy/arduboy_input.h for the Arduboy implementation.
//
// =============================================================================

#ifndef HAL_INPUT_H
#define HAL_INPUT_H

#include <stdint.h>

// =============================================================================
// Button Enum
// =============================================================================
// Platform-agnostic button identifiers.
// On Arduboy, these map to the 6 physical buttons.
// On a PC port, these would map to keyboard keys (e.g., WASD + J/K).

enum Button : uint8_t {
    BTN_UP    = 0,
    BTN_DOWN  = 1,
    BTN_LEFT  = 2,
    BTN_RIGHT = 3,
    BTN_A     = 4,
    BTN_B     = 5,

    BTN_COUNT = 6   // Total number of buttons (for array sizing)
};

// =============================================================================
// Input — API Contract
// =============================================================================
// Any platform input struct MUST implement these methods:
//
//   void poll();
//     Read the current state of all buttons from hardware.
//     Must be called ONCE at the start of each frame, before any pressed()
//     or justPressed() calls.
//     On Arduboy: wraps arduboy.pollButtons().
//
//   bool pressed(Button b) const;
//     Returns true if the button is CURRENTLY held down this frame.
//     Used for continuous actions: movement direction, acceleration, braking.
//     Example: while pressed(BTN_A), apply thrust every frame.
//
//   bool justPressed(Button b) const;
//     Returns true if the button was JUST pressed this frame (was not pressed
//     last frame, is pressed now). Returns false on subsequent frames even if
//     still held.
//     Used for one-shot actions: menu selection, starting the game.
//     Example: justPressed(BTN_A) to start game from title screen.
//
// =============================================================================

#endif // HAL_INPUT_H
