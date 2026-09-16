// =============================================================================
// arduboy_input.h — Arduboy2 Input Implementation
// =============================================================================
//
// PURPOSE:
//   Concrete implementation of the Input HAL contract for the Arduboy platform.
//   Maps the platform-agnostic Button enum to Arduboy2 button constants and
//   wraps pressed()/justPressed() calls.
//
// USAGE:
//   Created once in supermassive-whitehole.ino:
//     Arduboy2 arduboy;
//     ArduboyInput input(arduboy);
//
// IMPLEMENTS: All methods documented in src/hal/input.h
//
// BUTTON MAPPING:
//   BTN_UP    → UP_BUTTON
//   BTN_DOWN  → DOWN_BUTTON
//   BTN_LEFT  → LEFT_BUTTON
//   BTN_RIGHT → RIGHT_BUTTON
//   BTN_A     → A_BUTTON
//   BTN_B     → B_BUTTON
//
// =============================================================================

#ifndef ARDUBOY_INPUT_H
#define ARDUBOY_INPUT_H

#include <Arduboy2.h>
#include "../../hal/input.h"  // For Button enum

struct ArduboyInput {
    Arduboy2& hw;  // Reference to the Arduboy2 hardware instance

    // Constructor: takes a reference to the Arduboy2 instance from .ino
    ArduboyInput(Arduboy2& arduboyRef) : hw(arduboyRef) {}

    // Read current button state from hardware.
    // Must be called once at the start of each frame.
    void poll() {
        hw.pollButtons();
    }

    // Returns true if the button is currently held down
    bool pressed(Button b) const {
        return hw.pressed(mapButton(b));
    }

    // Returns true if the button was just pressed this frame (edge-triggered)
    bool justPressed(Button b) const {
        return hw.justPressed(mapButton(b));
    }

private:
    // Map platform-agnostic Button enum to Arduboy2 button constants
    static uint8_t mapButton(Button b) {
        switch (b) {
            case BTN_UP:    return UP_BUTTON;
            case BTN_DOWN:  return DOWN_BUTTON;
            case BTN_LEFT:  return LEFT_BUTTON;
            case BTN_RIGHT: return RIGHT_BUTTON;
            case BTN_A:     return A_BUTTON;
            case BTN_B:     return B_BUTTON;
            default:        return 0;
        }
    }
};

#endif // ARDUBOY_INPUT_H
