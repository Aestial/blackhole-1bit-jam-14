// =============================================================================
// renderer.h — Renderer HAL Interface Contract
// =============================================================================
//
// PURPOSE:
//   Defines the rendering API that any platform implementation MUST provide.
//   This file serves as the CONTRACT and DOCUMENTATION for the renderer.
//
// HOW IT WORKS:
//   This is NOT a virtual interface (no vtable overhead on AVR).
//   Instead, platform implementations (e.g., ArduboyRenderer) implement a
//   struct with the EXACT SAME method signatures listed below.
//   The game code accesses the renderer through a typedef in hal_types.h,
//   resolved at compile-time via #ifdef.
//
// FOR PLATFORM IMPLEMENTERS:
//   When creating a new renderer (e.g., for Raylib, SDL), create a struct
//   that implements ALL methods listed in the Renderer struct below.
//   The method signatures must match exactly. See:
//     src/platform/arduboy/arduboy_renderer.h for the Arduboy implementation.
//
// COORDINATE SYSTEM:
//   - Origin (0,0) is top-left of screen
//   - X increases rightward (0 to SCREEN_W-1 = 127)
//   - Y increases downward  (0 to SCREEN_H-1 = 63)
//   - Color: 0 = BLACK (pixel off), 1 = WHITE (pixel on)
//
// RENDERING PIPELINE (called each frame by Game):
//   1. clear()           — Fill screen with BLACK
//   2. draw*() calls     — Draw game objects (order matters: background first)
//   3. display()         — Push framebuffer to screen
//
// =============================================================================

#ifndef HAL_RENDERER_H
#define HAL_RENDERER_H

#include <stdint.h>

// =============================================================================
// Renderer — API Contract
// =============================================================================
// Any platform renderer struct MUST implement these methods:
//
//   void clear();
//     Clear the entire screen buffer to BLACK (color 0).
//     Called once at the start of each frame's render phase.
//
//   void display();
//     Push the screen buffer to the physical display.
//     Called once at the end of each frame's render phase.
//     On Arduboy, this sends the 1024-byte framebuffer to the OLED via SPI.
//
//   void drawPixel(int16_t x, int16_t y, uint8_t color);
//     Set a single pixel. color: 0=BLACK, 1=WHITE.
//     Used for fine detail and particle effects.
//
//   void drawRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color);
//     Draw a rectangle OUTLINE (1px border, hollow inside).
//     (x,y) = top-left corner, w = width, h = height.
//
//   void fillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color);
//     Draw a FILLED rectangle (solid).
//     Used for: burger placeholder (FOOD_BURGER_SIZE square), HUD backgrounds.
//
//   void drawCircle(int16_t x, int16_t y, uint8_t r, uint8_t color);
//     Draw a circle OUTLINE. (x,y) = center, r = radius.
//     Used for: blackhole concentric rings.
//
//   void fillCircle(int16_t x, int16_t y, uint8_t r, uint8_t color);
//     Draw a FILLED circle.
//     Used for: player placeholder (fat man), donut placeholder.
//
//   void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
//     Draw a line between two points.
//     Used for: background grid lines, blackhole convergence effect.
//
//   void drawTriangle(int16_t x0, int16_t y0,
//                     int16_t x1, int16_t y1,
//                     int16_t x2, int16_t y2, uint8_t color);
//     Draw a triangle OUTLINE from three vertices.
//
//   void fillTriangle(int16_t x0, int16_t y0,
//                     int16_t x1, int16_t y1,
//                     int16_t x2, int16_t y2, uint8_t color);
//     Draw a FILLED triangle.
//     Used for: pizza placeholder.
//
//   void setCursor(int16_t x, int16_t y);
//     Set the text cursor position for subsequent print() calls.
//     (x,y) = top-left of where the next character will be drawn.
//
//   void print(const char* text);
//     Print a null-terminated string at the current cursor position.
//     Uses the platform's built-in font (6x8 on Arduboy).
//     Used for: title screen text, "GAME OVER", button prompts.
//
//   void printNumber(int32_t number);
//     Print an integer as text at the current cursor position.
//     Used for: score display, high score.
//
// =============================================================================

#endif // HAL_RENDERER_H
