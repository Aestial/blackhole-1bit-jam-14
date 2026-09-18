# Distribution & Emulation Guide

This document describes the distribution architecture, automated build pipeline, and ProjectABE web emulator optimization for **Supermassive Whitehole** (1-Bit Game Jam 14).

---

## 1. Distribution Architecture

The project maintains a dedicated `dist/` directory that contains only release-ready binaries and the optimized web emulator:

```
dist/
├── supermassive-whitehole.hex # Production AVR HEX file (also mirrored to whitehole.hex)
├── supermassive-whitehole.elf # Debug symbols and memory analysis ELF binary
├── supermassive-whitehole-web.zip # Self-contained itch.io release bundle (~190 KB, mirrored to whitehole-web.zip)
├── README.md                 # Flashing and deployment quickstart guide
│   └── web/                      # Lean ProjectABE HTML5 emulator
│       ├── index.html            # Clean HTML5 entry point with responsive viewport
│       ├── app.js                # Core emulator engine (auto device detection, button latching, iframe overlay)
│       ├── style.css             # Clean layout (pixel-sharp canvas, touch styling, mobile overlay)
│       ├── ArduboyProject.hex    # Active game binary loaded by the emulator
│       ├── Arduboy (8).png       # High-contrast 626×1004 Arduboy handheld device casing
│       ├── logo.png              # 1-bit monochrome boot logo (1 KB)
│       ├── test_iframe.html      # Local developer harness simulating itch.io desktop & mobile embeds
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

## 2. ProjectABE Responsive Template Optimization

The stock ProjectABE emulator distribution contains over 6 MB of extraneous assets (unused skins, 30+ Ace editor themes, demo games, unneeded libraries).

For production distribution, `dist/web/` was optimized down to **~190 KB** compressed with the following enhancements:

### Responsive Device Detection & Layout Modes (GB Studio Behavior)
Matching the seamless device responsiveness of **GB Studio HTML exports**, the emulator automatically adapts its layout depending on whether the user is on desktop or mobile:

1. **Desktop Mode (`bare` / `BareFit`)**:
   - Automatically selected when accessing from desktop browsers (`width > 768px` and no mobile user-agent).
   - Renders the pure 128×64 OLED screen scaled 2:1 with black letterboxing, pixel-perfect rendering, and zero on-screen button clutter.
   - Fully controllable via keyboard (`Arrow Keys`, `Z` for Button A, `X` for Button B).

2. **Mobile Handheld Mode (`arduboy`)**:
   - Automatically selected when accessing from mobile devices (`Android`, `iPhone`, `iPad`, or `width <= 768px` in portrait).
   - Renders the full handheld Arduboy shell using high-contrast **`Arduboy (8).png`** (626×1004).
   - Accurately positioned hitboxes for the D-pad and Action Buttons with touch-optimized event handling and visual active feedback.
   - **Touch Input Latching (~55ms)**: Guarantees even instantaneous sub-frame taps are detected by the Arduboy 60 FPS polling cycle without missed inputs.
   - Prevents scroll/zoom gestures with `touch-action: none`.

3. **itch.io Mobile Iframe Handling**:
   - When embedded inside an iframe on a mobile device (`window !== window.top`), touch scrolling and iframe gestures can disrupt gameplay.
   - The emulator displays a sleek, retro **"TAP TO PLAY FULLSCREEN"** overlay. Tapping it opens the mobile standalone emulator in a dedicated browser tab for an uninhibited handheld gaming experience.
   - In desktop iframes, the game runs directly inside the itch.io embed with unrestricted keyboard controls.

4. **URL Overrides & In-Browser Toggle**:
   - Force Desktop: `?skin=bare` or `?mode=desktop`
   - Force Mobile: `?skin=arduboy` or `?mode=mobile`
   - Press <kbd>F3</kbd> in a desktop browser at any time to toggle skins.

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
3. Copies `build/supermassive-whitehole.ino.hex` to `dist/supermassive-whitehole.hex` and `dist/web/ArduboyProject.hex`.
4. Checks if an HTTP server is already running on the requested port; if not, starts a lightweight background Python HTTP server (`python3 -m http.server <port> --directory dist/web`).
5. Opens the browser via `xdg-open` directly to the running game.

---

## 4. itch.io Submission (1-Bit Game Jam 14)

To submit the HTML5 web build:
1. Run `./build.sh --package --build-only` to generate `dist/supermassive-whitehole-web.zip`.
2. On your itch.io project dashboard:
   - Set **Kind of project** to **HTML**.
   - Upload `dist/supermassive-whitehole-web.zip`.
   - Check **"This file will be played in the browser"**.
   - Set viewport dimensions to **640 × 320** (for game screen mode) or **800 × 600** (if you prefer displaying the Arduboy casing).
   - Enable **Fullscreen button** and **Mobile friendly**.
3. Save and preview.
