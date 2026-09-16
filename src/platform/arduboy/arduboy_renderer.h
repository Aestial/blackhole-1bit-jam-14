// =============================================================================
// arduboy_renderer.h — Arduboy2 Renderer Implementation
// =============================================================================
//
// PURPOSE:
//   Concrete implementation of the Renderer HAL contract for the Arduboy
//   platform. Wraps Arduboy2 library draw calls.
//
// USAGE:
//   Created once in supermassive-whitehole.ino with a reference to the Arduboy2 instance:
//     Arduboy2 arduboy;
//     ArduboyRenderer renderer(arduboy);
//
// IMPLEMENTS: All methods documented in src/hal/renderer.h
//
// =============================================================================

#ifndef ARDUBOY_RENDERER_H
#define ARDUBOY_RENDERER_H

#include <Arduboy2.h>

struct ArduboyRenderer {
    Arduboy2& hw;  // Reference to the Arduboy2 hardware instance

    // Constructor: takes a reference to the Arduboy2 instance from .ino
    ArduboyRenderer(Arduboy2& arduboyRef) : hw(arduboyRef) {}

    // Clear the screen buffer to BLACK
    void clear() {
        hw.clear();
    }

    // Push the screen buffer to the OLED display
    void display() {
        hw.display();
    }

    // Set a single pixel
    void drawPixel(int16_t x, int16_t y, uint8_t color) {
        hw.drawPixel(x, y, color);
    }

    // Draw rectangle outline
    void drawRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color) {
        hw.drawRect(x, y, w, h, color);
    }

    // Draw filled rectangle
    void fillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color) {
        hw.fillRect(x, y, w, h, color);
    }

    // Draw circle outline
    void drawCircle(int16_t x, int16_t y, uint8_t r, uint8_t color) {
        hw.drawCircle(x, y, r, color);
    }

    // Draw filled circle
    void fillCircle(int16_t x, int16_t y, uint8_t r, uint8_t color) {
        hw.fillCircle(x, y, r, color);
    }

    // Draw a line between two points
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
        hw.drawLine(x0, y0, x1, y1, color);
    }

    // Draw triangle outline from three vertices
    void drawTriangle(int16_t x0, int16_t y0,
                      int16_t x1, int16_t y1,
                      int16_t x2, int16_t y2, uint8_t color) {
        hw.drawTriangle(x0, y0, x1, y1, x2, y2, color);
    }

    // Draw filled triangle from three vertices
    void fillTriangle(int16_t x0, int16_t y0,
                      int16_t x1, int16_t y1,
                      int16_t x2, int16_t y2, uint8_t color) {
        hw.fillTriangle(x0, y0, x1, y1, x2, y2, color);
    }

    // Set text cursor position for subsequent print() calls
    void setCursor(int16_t x, int16_t y) {
        hw.setCursor(x, y);
    }

    // Print a null-terminated string at the current cursor position
    void print(const char* text) {
        hw.print(text);
    }

    // Print an integer as text at the current cursor position
    void printNumber(int32_t number) {
        hw.print(number);
    }

    // Draw sprite using Sprites::drawSelfMasked (1s are white, 0s are transparent)
    void drawSelfMasked(int16_t x, int16_t y, const uint8_t* bitmap, uint8_t frame) {
        Sprites::drawSelfMasked(x, y, bitmap, frame);
    }

    // Draw sprite using Sprites::drawOverwrite (replaces buffer completely)
    void drawOverwrite(int16_t x, int16_t y, const uint8_t* bitmap, uint8_t frame) {
        Sprites::drawOverwrite(x, y, bitmap, frame);
    }
};

#endif // ARDUBOY_RENDERER_H
