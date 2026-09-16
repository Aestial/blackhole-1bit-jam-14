# Distribution & Emulation Guide

This document describes the distribution architecture, automated build pipeline, and ProjectABE web emulator optimization for **Supermassive Blackhole** (1-Bit Game Jam 14).

---

## 1. Distribution Architecture

The project maintains a dedicated `dist/` directory that contains only release-ready binaries and the optimized web emulator:

```
blackhole/
├── dist/
│   ├── blackhole.hex             # Production AVR HEX file for hardware flashing
│   ├── blackhole.elf             # Debug symbols and memory analysis ELF binary
│   ├── blackhole-web.zip         # Self-contained itch.io release bundle (~470 KB)
│   ├── README.md                 # Flashing and deployment quickstart guide
│   └── web/                      # Lean ProjectABE HTML5 emulator (~820 KB total)
│       ├── index.html            # Clean HTML5 entry point (no external editor dependencies)
│       ├── app.js                # Core emulator engine (autorun & dual-skin enabled)
│       ├── style.css             # Clean layout (debugger overlay hidden, pixel-sharp canvas)
│       ├── ArduboyProject.hex    # Active game binary loaded by the emulator
│       ├── Arduboy-off.png       # Compact 393×624 Arduboy device casing skin
│       ├── logo.png              # 1-bit monochrome boot logo (1 KB)
│       └── layouts/              # Minimal layout templates
│           ├── Sim.html          # Simulator DOM tree (canvas, audio, buttons)
│           ├── Env.html          # App container
│           └── Splash.html       # Boot splash
├── scripts/
│   ├── build_and_run.sh          # Main Linux build and emulation pipeline
│   └── package_dist.sh           # Packager for itch.io zip archive
├── build.sh                      # Root convenience shortcut to scripts/build_and_run.sh
└── html5/                        # Upstream ProjectABE reference directory
```

---

## 2. ProjectABE Template Optimization

The stock ProjectABE emulator distribution contains over 6 MB of extraneous assets (unused skins like Pipboy and Tamagotchi, 30+ Ace editor themes, demo games, unneeded libraries).

For production distribution, `dist/web/` was optimized down to **~820 KB** (and **~470 KB** compressed) with the following enhancements:

### Key Optimizations
1. **Unused Asset Stripping**: Removed 10 large 626×1004 skins (`Arduboy (0..9).png`), Pipboy, Tamagotchi, Microcard textures, unused ace themes, and demo `.arduboy` binaries.
2. **Debugger Overlay Suppression**: The floating unstyled `#ideContainer` panel is suppressed by default (`display: none !important`), eliminating visual clutter over the game.
3. **Automatic Game Boot**: If no query parameters (`?hex=...`) are provided in the URL, `app.js` defaults `url` to `'ArduboyProject.hex'`, ensuring the game immediately launches upon opening `index.html`.
4. **Dual Display Skin Modes**:
   - **Pure Game Screen (`bare` / `BareFit`)**: Default mode. The 128×64 OLED screen scales to fit the browser viewport in a strict 2:1 aspect ratio with black letterboxing, razor-sharp pixel rendering, and zero device casing.
   - **Compact Arduboy Skin (`arduboy`)**: Uses the smallest official Arduboy casing (`Arduboy-off.png`, 393×624). The screen fits accurately within the OLED bezel, and on-screen D-pad / action buttons are fully functional for touch/mouse input.
   - **In-Browser Toggle**: Press <kbd>F3</kbd> at any time to toggle between skins.

---

## 3. Automated Build & Emulation Workflow

### Building and Running
The root `./build.sh` script automates compilation, artifact synchronization, web server management, and browser launching:

```bash
# Compile and run with clean game screen (default)
./build.sh

# Compile and run with the compact Arduboy device skin
./build.sh --skin arduboy

# Build and sync binaries only (no web server / browser launch)
./build.sh --build-only

# Build and generate itch.io release package
./build.sh --package --build-only

# Specify a custom HTTP port (default is 8000)
./build.sh --port 8080

# Clean build directory before compiling
./build.sh --clean
```

### What `build.sh` Automates
1. Calls `arduino-cli compile --fqbn arduboy-homemade:avr:arduboy ./` to compile the C++ source.
2. Checks exit code; fails fast with formatted error logs if compilation encounters errors.
3. Copies `build/blackhole.ino.hex` to `dist/blackhole.hex` and `dist/web/ArduboyProject.hex`.
4. Checks if an HTTP server is already running on the requested port; if not, starts a lightweight background Python HTTP server (`python3 -m http.server <port> --directory dist/web`).
5. Opens the browser via `xdg-open` directly to the running game.

---

## 4. itch.io Submission (1-Bit Game Jam 14)

To submit the HTML5 web build:
1. Run `./build.sh --package --build-only` to generate `dist/blackhole-web.zip`.
2. On your itch.io project dashboard:
   - Set **Kind of project** to **HTML**.
   - Upload `dist/blackhole-web.zip`.
   - Check **"This file will be played in the browser"**.
   - Set viewport dimensions to **640 × 320** (for game screen mode) or **800 × 600** (if you prefer displaying the Arduboy casing).
   - Enable **Fullscreen button** and **Mobile friendly**.
3. Save and preview.
