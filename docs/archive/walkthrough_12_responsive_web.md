# Responsive Web Emulator & itch.io Mobile Device Detection

## Summary of Accomplishments

We have finalized the responsive layout and device-detection integration for the ProjectABE web emulator on itch.io, matching the user experience of **GB Studio HTML exports**:

1. **Automatic Device Detection**:
   - **Desktop**: Automatically defaults to pure 2:1 OLED screen (`BareFit`) with crisp pixel scaling, no virtual buttons, and direct keyboard input.
   - **Mobile Devices**: Automatically detects mobile phones and tablets (`/Android|iPhone|iPad|iPod|Mobile/i` or `width <= 768px` in portrait) and defaults to the handheld casing skin (`Arduboy (8).png`) with on-screen D-pad and Action buttons.
   - **URL Parameter Overrides**: Supports `?mode=desktop` / `?skin=bare` and `?mode=mobile` / `?skin=arduboy` for debugging and user preference.

2. **Direct, Streamlined Mobile Embed (Zero Redundant Prompts)**:
   - Since itch.io mobile game pages already present a native browser action button to start/fullscreen embedded content, the emulator avoids any redundant internal "Tap to Play" overlay.
   - On mobile, the emulator immediately renders the full Arduboy handheld skin (`Arduboy (8).png`) with responsive on-screen D-pad and Action buttons, allowing players to jump straight into the game.
   - On desktop, the embed directly renders the clean OLED display and receives keyboard inputs (`Arrow Keys`, `Z` for Button A, `X` for Button B) without blocking or interception.

3. **Touch Input Latching (~55ms) & Gesture Prevention**:
   - **55ms Input Latch**: When tapping virtual buttons on mobile touchscreens, rapid micro-taps (10–20ms) are latched for at least 55ms (~3.3 frames at 60 FPS) to ensure the Arduboy AVR polling loop (`arduboy.pollButtons()`) reliably captures the input without missed taps.
   - Handlers for `touchstart`, `touchend`, `touchcancel`, `mousedown`, `mouseup`, and `mouseleave` call `evt.preventDefault()` and set `touch-action: none` to prevent double-tap zoom, gesture delays, and stuck buttons.

4. **Testing Framework & Packaging**:
   - [dist/web/test_iframe.html](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/test_iframe.html): Interactive simulation harness to test itch.io desktop embeds (640×360) and mobile embeds (375×667).
   - [scripts/package_dist.sh](file:///home/dorito/Developer/arduboy/supermassive-whitehole/scripts/package_dist.sh): Builds clean zip distributions (`dist/supermassive-whitehole-web.zip` ~189 KB), excluding test files while including the `Arduboy (8).png` skin.
   - Updated [docs/TODO.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/TODO.md) and [docs/distribution.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/distribution.md).

---

## Verification & Screenshots

### 1. Desktop itch.io Iframe Simulation
Keyboard inputs (`KeyZ` / Button A) pass directly into the game canvas, immediately launching and playing the game inside the embed.

| Desktop Embed Title Screen | Active Gameplay in Iframe |
|---|---|
| ![Desktop Iframe Loaded](/home/dorito/.gemini/antigravity-ide/brain/aa8e6bd4-7a65-4527-8fb3-7d9f8317f37c/iframe_desktop_title_1789704746410.png) | ![Desktop Iframe Gameplay](/home/dorito/.gemini/antigravity-ide/brain/aa8e6bd4-7a65-4527-8fb3-7d9f8317f37c/iframe_desktop_gameplay_1789704835865.png) |

---

### 2. Direct Mobile itch.io Iframe Simulation (No Redundant Overlay)
When embedded on mobile, the emulator immediately displays the handheld casing with responsive touch controls:

| Mobile Embed Title Screen | Direct Gameplay via On-Screen Button A |
|---|---|
| ![Mobile Direct Skin](/home/dorito/.gemini/antigravity-ide/brain/aa8e6bd4-7a65-4527-8fb3-7d9f8317f37c/mobile_direct_skin_in_iframe_1789711016181.png) | ![Mobile Direct Gameplay](/home/dorito/.gemini/antigravity-ide/brain/aa8e6bd4-7a65-4527-8fb3-7d9f8317f37c/mobile_direct_gameplay_in_iframe_1789711138510.png) |

---

## Updated Files

- [dist/web/app.js](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/app.js) and [html5/app.js](file:///home/dorito/Developer/arduboy/supermassive-whitehole/html5/app.js):
  - Module 19 (`BTN.js`): 55ms touch latching, `touchcancel`/`mouseleave` safety, and `preventDefault()`.
  - Module 7 (`App.js`): Streamlined `initIFrame()` without redundant overlays or input blocking.
  - Module 41 (`Arduboy.js`): Calibrated hitboxes and skin coordinates for `Arduboy (8).png`.
  - Module 51 (`boot`): Responsive device detection logic.
- [dist/web/style.css](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/style.css) and [html5/style.css](file:///home/dorito/Developer/arduboy/supermassive-whitehole/html5/style.css):
  - Set `touch-action: none` on `#simContainer btn`.
  - Removed obsolete overlay styling.
- [dist/web/index.html](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/index.html) and [html5/index.html](file:///home/dorito/Developer/arduboy/supermassive-whitehole/html5/index.html):
  - Cache buster bumped to `v=0.3.3`.
- [dist/web/test_iframe.html](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/test_iframe.html):
  - Interactive simulator harness for switching between Desktop and Mobile embed environments.
- [scripts/package_dist.sh](file:///home/dorito/Developer/arduboy/supermassive-whitehole/scripts/package_dist.sh):
  - Excludes `test_*.html` and temporary files from the production web release zip.
- [docs/distribution.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/distribution.md):
  - Updated documentation for responsive layouts and itch.io embed options.
- [docs/TODO.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/TODO.md):
  - Marked device detection task as completed and formatted upstream contribution item.
