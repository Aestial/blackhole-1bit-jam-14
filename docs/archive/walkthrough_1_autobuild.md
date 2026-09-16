# Distribution Architecture & Emulation Walkthrough

We have set up an automated distribution pipeline and an optimized ProjectABE web emulator for **Supermassive Blackhole**.

---

## What Was Created and Updated

### 1. Build & Emulation Automation (`build.sh`)
- Created [build.sh](file:///home/dorito/Developer/arduboy/supermassive-whitehole/build.sh) and [scripts/build_and_run.sh](file:///home/dorito/Developer/arduboy/supermassive-whitehole/scripts/build_and_run.sh).
- Single command workflow:
  - Compiles the sketch with `arduino-cli` using FQBN `arduboy-homemade:avr:arduboy`.
  - Automatically synchronizes `dist/blackhole.hex` and `dist/web/ArduboyProject.hex`.
  - Checks if a local web server is running on the target port (default `8000`), launching a background Python server if needed.
  - Launches the browser directly into the running game via `xdg-open`.
- Supports `--skin bare` (default), `--skin arduboy`, `--build-only`, `--port <port>`, `--clean`, and `--package`.

### 2. Lean Web Distribution (`dist/web/`)
- Stripped unused assets from the original 6 MB template down to **820 KB** (and **470 KB** compressed).
- Eliminated 10 large unused skins, 30+ Ace themes, and demo `.arduboy` files.
- Kept only essential emulator assets:
  - `dist/web/index.html` (clean entry point, title updated)
  - `dist/web/app.js` (core emulator engine)
  - `dist/web/style.css` (debugger overlay suppressed, crisp pixel rendering)
  - `dist/web/ArduboyProject.hex` (game binary)
  - `dist/web/Arduboy-off.png` (compact 393×624 device casing)
  - `dist/web/logo.png` (splash logo)
  - `dist/web/layouts/` (`Sim.html`, `Env.html`, `Splash.html`)

### 3. ProjectABE Template Display Modes
- **Mode 1: Pure Game Screen (`--skin bare`, Default)**:
  - Screen scales dynamically in a clean 2:1 aspect ratio with black letterboxing.
  - No device casing, no borders, no floating debugger UI.
- **Mode 2: Compact Arduboy Skin (`--skin arduboy`)**:
  - Uses the smallest official casing (`Arduboy-off.png`, 393×624).
  - Screen aligns inside the OLED bezel with functional on-screen D-pad and A/B buttons.
- **Dynamic Toggle**: Press <kbd>F3</kbd> in-browser at any time to switch skins.

### 4. Release Packaging & Documentation
- Created [scripts/package_dist.sh](file:///home/dorito/Developer/arduboy/supermassive-whitehole/scripts/package_dist.sh) to build `dist/blackhole-web.zip` for itch.io 1-Bit Game Jam uploads.
- Created [dist/README.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/README.md) and [docs/distribution.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/distribution.md).

---

## Visual Verification

### 1. Pure Game Screen Mode (Default)
Clean 2:1 OLED display with pixel-perfect font rendering, no bezels or debugger clutter:

![Pure Game Screen Mode](/home/dorito/.gemini/antigravity-ide/brain/f34e6c27-17d7-475b-8882-acaebefd558a/game_screen_verify_1789522201876.png)

### 2. Smallest Arduboy Skin Mode (`Arduboy-off.png`)
Compact 393×624 device casing with aligned OLED screen and interactive controls:

![Arduboy-off Skin Mode](/home/dorito/.gemini/antigravity-ide/brain/f34e6c27-17d7-475b-8882-acaebefd558a/arduboy_skin_check_1789522369619.png)

### 3. Gameplay & State Machine Execution
Verified button input (A button / Z key) transitions from title screen to active gameplay:

![Gameplay Execution](/home/dorito/.gemini/antigravity-ide/brain/f34e6c27-17d7-475b-8882-acaebefd558a/gameplay_state_after_press_1789522446244.png)

---

## Commands Summary

```bash
# Build sketch and launch pure game screen emulator
./build.sh

# Build sketch and launch with compact Arduboy casing
./build.sh --skin arduboy

# Build hardware and web binaries only
./build.sh --build-only

# Build and generate release zip for itch.io
./build.sh --package --build-only
```
