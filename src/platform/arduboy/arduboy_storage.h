// =============================================================================
// arduboy_storage.h — Arduboy EEPROM Storage Implementation
// =============================================================================
//
// PURPOSE:
//   Concrete implementation of the Storage HAL contract for the Arduboy
//   platform. Uses AVR EEPROM for persistent high score storage.
//
// USAGE:
//   Created once in blackhole.ino:
//     ArduboyStorage storage;
//
// IMPLEMENTS: All methods documented in src/hal/storage.h
//
// EEPROM NOTES:
//   - Arduboy has 1 KB of EEPROM (addresses 0-1023)
//   - System reserves addresses 0-15; we use EEPROM_HIGH_SCORE_ADDR (16)
//   - High score is stored as uint16_t (2 bytes at addresses 16-17)
//   - EEPROM starts as 0xFF on blank chips; loadHighScore() handles this
//     by treating 0xFFFF as "no saved score" and returning 0
//   - EEPROM has ~100,000 write cycles per cell — plenty for high scores
//   - EEPROM.get()/EEPROM.put() handle multi-byte read/write automatically
//
// =============================================================================

#ifndef ARDUBOY_STORAGE_H
#define ARDUBOY_STORAGE_H

#include <EEPROM.h>
#include "../../game/config.h"  // For EEPROM_HIGH_SCORE_ADDR

struct ArduboyStorage {
    // Save high score to EEPROM
    // Only writes if the value has actually changed (EEPROM.put does this
    // internally, avoiding unnecessary write cycles)
    void saveHighScore(uint16_t score) {
        EEPROM.put(EEPROM_HIGH_SCORE_ADDR, score);
    }

    // Load high score from EEPROM
    // Returns 0 if EEPROM is blank (0xFFFF) — first boot on a new device
    uint16_t loadHighScore() {
        uint16_t score = 0;
        EEPROM.get(EEPROM_HIGH_SCORE_ADDR, score);

        // Blank EEPROM reads as 0xFFFF — treat as "no saved score"
        if (score == 0xFFFF) {
            return 0;
        }
        return score;
    }
};

#endif // ARDUBOY_STORAGE_H
