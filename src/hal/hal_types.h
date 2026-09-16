// =============================================================================
// hal_types.h — Platform Type Selector (Compile-Time HAL Binding)
// =============================================================================
//
// PURPOSE:
//   Resolves the abstract HAL type names (HalRenderer, HalInput, HalStorage)
//   to concrete platform implementations based on the PLATFORM_* define
//   set in config.h.
//
// HOW IT WORKS:
//   1. config.h defines PLATFORM_ARDUBOY (or another platform constant)
//   2. This file includes the matching platform headers
//   3. Creates typedefs: HalRenderer, HalInput, HalStorage
//   4. Game code includes this file and uses the typedef'd names
//
//   When porting to a new platform:
//   1. Create new platform files in src/platform/<name>/
//   2. Add an #elif block here for the new PLATFORM_* define
//   3. Change the #define in config.h
//   4. Recompile — game logic code doesn't change at all!
//
// ZERO OVERHEAD:
//   This is pure compile-time resolution. No virtual dispatch, no function
//   pointers, no vtables. The compiler sees the concrete types directly.
//
// WHICH FILES INCLUDE THIS:
//   Only game.h/game.cpp (the state machine + rendering bridge).
//   All other game logic files (player.h, entity.h, world.h, physics.h)
//   are 100% platform-independent and never include this file.
//
// =============================================================================

#ifndef HAL_TYPES_H
#define HAL_TYPES_H

#include "../game/config.h"  // For PLATFORM_* define

// =============================================================================
// Platform: Arduboy
// =============================================================================
#ifdef PLATFORM_ARDUBOY

#include "../platform/arduboy/arduboy_renderer.h"
#include "../platform/arduboy/arduboy_input.h"
#include "../platform/arduboy/arduboy_storage.h"

typedef ArduboyRenderer HalRenderer;
typedef ArduboyInput    HalInput;
typedef ArduboyStorage  HalStorage;

// =============================================================================
// Platform: Raylib (future — example skeleton for porting)
// =============================================================================
// #elif defined(PLATFORM_RAYLIB)
//
// #include "../platform/raylib/raylib_renderer.h"
// #include "../platform/raylib/raylib_input.h"
// #include "../platform/raylib/raylib_storage.h"
//
// typedef RaylibRenderer HalRenderer;
// typedef RaylibInput    HalInput;
// typedef RaylibStorage  HalStorage;

// =============================================================================
// Platform: SDL2 (future — another possible port)
// =============================================================================
// #elif defined(PLATFORM_SDL2)
//
// #include "../platform/sdl2/sdl2_renderer.h"
// #include "../platform/sdl2/sdl2_input.h"
// #include "../platform/sdl2/sdl2_storage.h"
//
// typedef SDL2Renderer HalRenderer;
// typedef SDL2Input    HalInput;
// typedef SDL2Storage  HalStorage;

#else
#error "No platform defined! Set PLATFORM_ARDUBOY (or another) in config.h"
#endif

#endif // HAL_TYPES_H
