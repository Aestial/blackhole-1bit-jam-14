// =============================================================================
// storage.h — Storage HAL Interface Contract
// =============================================================================
//
// PURPOSE:
//   Defines the persistent storage API for saving/loading game data.
//   Currently only used for high score persistence.
//
// ARDUBOY HARDWARE:
//   1 KB EEPROM, byte-addressable, survives power cycles.
//   Arduboy system reserves addresses 0-15. We use address 16+ (see config.h).
//   EEPROM has ~100,000 write cycles per cell — more than enough for high scores.
//
// DATA FORMAT:
//   High score is stored as a uint16_t (2 bytes) at EEPROM_HIGH_SCORE_ADDR.
//   Max value: 65535. If the stored value looks corrupted (e.g., 0xFFFF on
//   first boot with blank EEPROM), the implementation should return 0.
//
// FOR PLATFORM IMPLEMENTERS:
//   - Arduboy: use EEPROM.get() / EEPROM.put() (handles multi-byte read/write)
//   - PC/Raylib: use a small binary file or JSON to persist the score
//   See: src/platform/arduboy/arduboy_storage.h for the Arduboy implementation.
//
// =============================================================================

#ifndef HAL_STORAGE_H
#define HAL_STORAGE_H

#include <stdint.h>

// =============================================================================
// Storage — API Contract
// =============================================================================
// Any platform storage struct MUST implement these methods:
//
//   void saveHighScore(uint16_t score);
//     Persist the given score to non-volatile storage.
//     Called when the player achieves a new high score at game over.
//     On Arduboy: EEPROM.put(EEPROM_HIGH_SCORE_ADDR, score)
//
//   uint16_t loadHighScore();
//     Read and return the persisted high score.
//     Called once at game init.
//     If no score has been saved yet (blank EEPROM = 0xFFFF), return 0.
//     On Arduboy: EEPROM.get(EEPROM_HIGH_SCORE_ADDR, score)
//
// =============================================================================

#endif // HAL_STORAGE_H
