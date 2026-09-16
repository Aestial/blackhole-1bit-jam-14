# Supermassive Whitehole — Build Script Fix & Renaming Audit Walkthrough

This document details the resolution of the build failure, the project-wide name migration audit, the storefront asset organization, and the verification of the custom sprite system.

---

## 1. Root Cause & Build Pipeline Fixes

### The Problem
When running `./build.sh`, compilation succeeded with `arduino-cli compile --fqbn arduboy-homemade:avr:arduboy ./`, but artifact synchronization immediately failed with:
```
[INFO] Synchronizing distribution artifacts...
[ERROR] Built hex file not found in /home/dorito/Developer/arduboy/supermassive-whitehole/build
```

### The Root Cause
By default, `arduino-cli compile` outputs compiled binaries to the system cache (`~/.cache/arduino/sketches/<hash>/`), not to `./build`, unless `--output-dir` is explicitly passed. When the directory was renamed or cleaned, `./build` was empty, causing `scripts/build_and_run.sh` to fail.

Additionally, a stale Python HTTP server process from the previous project path (`/home/dorito/Developer/arduboy/blackhole`) remained bound to port 8000, serving HTTP 404 responses for any emulator requests.

### The Fixes
1. **Direct Output to Build Directory**:
   In [scripts/build_and_run.sh](file:///home/dorito/Developer/arduboy/supermassive-whitehole/scripts/build_and_run.sh), added `--output-dir "${ROOT_DIR}/build"` to `arduino-cli compile`. This guarantees that `${SKETCH_NAME}.ino.hex` and `${SKETCH_NAME}.ino.elf` are always written to `build/`.
2. **Bootloader Exclusion**:
   Updated fallback hex discovery to ignore `*.with_bootloader.hex`.
3. **Artifact Aliases**:
   Generated both primary binaries (`supermassive-whitehole.hex`, `supermassive-whitehole-web.zip`) and compatibility mirrors (`whitehole.hex`, `whitehole-web.zip`, `blackhole-web.zip`).
4. **Stale Server Detection & Persistence**:
   Enhanced server startup to test HTTP response codes (killing stale 404 servers) and used `nohup` so background servers survive script termination.
5. **Emulator Fresh Fetch**:
   In [dist/web/app.js](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/app.js) and [html5/app.js](file:///home/dorito/Developer/arduboy/supermassive-whitehole/html5/app.js), bypassed stale IndexedDB `build.hex` cache so the emulator always fetches the latest binary from disk.
6. **Clean Presentation**:
   Disabled `debuggerEnabled` in `app.js` and added CSS rules in `style.css` to hide empty background images, presenting a distraction-free 2:1 OLED display.

---

## 2. Project Name Migration Audit

Every component and document across the repository was audited:

| Component / Area | Status | Changes Made |
|---|---|---|
| **Entry Point** | Verified | Renamed `blackhole.ino` to `supermassive-whitehole.ino` |
| **HAL Headers** | Fixed | Updated obsolete `blackhole.ino` comments in [arduboy_storage.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/platform/arduboy/arduboy_storage.h), [arduboy_input.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/platform/arduboy/arduboy_input.h), and [arduboy_renderer.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/platform/arduboy/arduboy_renderer.h) |
| **Game Engine** | Fixed | Updated entry reference in [src/game/game.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.h) |
| **Documentation Links** | Fixed | Corrected obsolete `/home/dorito/Developer/arduboy/blackhole` paths across all `docs/*.md` files |
| **Build & Packaging Scripts** | Fixed | Updated `build_and_run.sh` and `package_dist.sh` to output `supermassive-whitehole` targets with mirrors |
| **Distribution Manuals** | Fixed | Updated [dist/README.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/README.md) and [docs/distribution.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/distribution.md) |
| **Agent History** | Intact | All previous transcripts, logs, and artifacts are safely preserved in `~/.gemini/antigravity-ide/brain/dd9e27b9-42f6-461c-8ec8-58cb2fbeb696` and `f34e6c27-17d7-475b-8882-acaebefd558a` |

---

## 3. Storefront & Distribution Assets

Created dedicated folder [dist/assets/](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/assets/) with [dist/assets/README.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/assets/README.md) to store itch.io publishing assets:

- `dist/assets/screenshot_title.png`: Title screen capture showing `SUPERMASSIVE WHITEHOLE`
- `dist/assets/screenshot_gameplay.png`: Active gameplay capture showing sprites in action
- Placeholders & instructions for `cover.png` (630×500 px) and `favicon.png` (32×32 / 64×64 px)

---

## 4. Sprite System Verification

Visual verification of the compiled game running in the ProjectABE HTML5 emulator:

### 1. Title Screen
Shows `SUPERMASSIVE WHITEHOLE`, high score tracking, and input prompt in clean 2:1 OLED resolution:

![Title Screen](/home/dorito/.gemini/antigravity-ide/brain/af03ebb1-dd3e-464b-b547-711706f41dba/screenshot_title.png)

### 2. Active Gameplay & Sprites
Shows the custom 16×16 player character fleeing the animated 32×32 swirling whitehole entity with spiral arms on the scrolling dot grid:

![Gameplay with Custom Sprites](/home/dorito/.gemini/antigravity-ide/brain/af03ebb1-dd3e-464b-b547-711706f41dba/screenshot_gameplay.png)

---

## 5. How to Run & Test

```bash
# Build and test in web emulator (clean 2:1 display)
./build.sh

# Run emulator with compact Arduboy casing skin
./build.sh --skin arduboy

# Build binaries only (fast check)
./build.sh --build-only

# Build and package itch.io release bundle
./build.sh --package --build-only
```
