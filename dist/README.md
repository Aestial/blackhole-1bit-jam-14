# Supermassive Whitehole — Distribution & Release Artifacts

This directory contains release binaries and web emulation packages for **Supermassive Whitehole** (1-Bit Game Jam 14).

---

## Directory Contents

| File / Folder | Purpose |
|---|---|
| `supermassive-whitehole.hex` | Production Intel HEX binary for flashing to physical Arduboy hardware (also mirrored to `whitehole.hex`) |
| `supermassive-whitehole.elf` | ELF binary with debug symbols (for disassembly, stack analysis, memory inspection) |
| `supermassive-whitehole-web.zip` | Self-contained HTML5 web player package ready for 1-click upload to [itch.io](https://itch.io) (also mirrored to `whitehole-web.zip`) |
| `web/` | Minimal, optimized ProjectABE HTML5 web emulator directory (only essential assets, ~820 KB) |

---

## 1. Flashing to Physical Arduboy Hardware

### Option A: Using `arduino-cli`
Connect the Arduboy via micro-USB and run:
```bash
# Find your port (e.g., /dev/ttyACM0)
arduino-cli board list

# Flash binary
arduino-cli upload -p /dev/ttyACM0 --fqbn arduboy-homemade:avr:arduboy --input-file dist/supermassive-whitehole.hex
```

### Option B: Using Arduboy Quick Flasher / Web Flashers
Open any browser-based Arduboy uploader (e.g., WebUSB Arduboy flasher) and drag-and-drop `dist/supermassive-whitehole.hex`.

---

## 2. Uploading to itch.io (1-Bit Game Jam 14)

1. Navigate to your project on **itch.io** and click **Edit game**.
2. Under **Kind of project**, select **HTML** (*You have a ZIP or HTML file that will be played in the browser*).
3. In the **Uploads** section, upload `dist/supermassive-whitehole-web.zip`.
4. Check the box: **"This file will be played in the browser"**.
5. Recommended Embed Options:
   - **Viewport dimensions**: `640` × `320` (or `800` × `600` if using Arduboy casing)
   - **Orientation**: Default / Landscape
   - **Mobile friendly**: Checked
   - **Automatically start on page load**: Checked
   - **Fullscreen button**: Checked

---

## 3. Local Web Emulation & Testing

To test the web build locally:
```bash
# Pure game screen (no skin, 2:1 aspect ratio)
./build.sh

# Compact Arduboy casing skin (Arduboy-off.png)
./build.sh --skin arduboy

# Build binaries only (no browser)
./build.sh --build-only

# Rebuild and package release zip
./build.sh --package --build-only
```

Inside the browser, press **F3** at any time to toggle between display skins.
