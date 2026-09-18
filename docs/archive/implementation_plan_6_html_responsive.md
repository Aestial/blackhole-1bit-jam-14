# Automatic Desktop/Mobile Device Detection & Arduboy Skin Integration Plan

## Goal
Implement automatic device detection and responsive display modes for the ProjectABE web emulator on itch.io, matching the behavior of GB Studio HTML exports:
1. **Desktop Mode**: Render the pure, clean game screen (no skin, no virtual buttons, 2:1 OLED scaling) with unrestricted keyboard inputs, fully playable inside the itch.io embed or fullscreen.
2. **Mobile Mode (Portrait)**: Render the full Arduboy handheld skin using the high-contrast **`Arduboy (8).png`** with working touch controls for D-Pad and A/B buttons.
3. **itch.io Mobile Iframe Handling**: When loaded in an iframe on mobile, provide a clean "Tap to Play in Full Screen" prompt that opens the standalone mobile emulator in a full browser tab, ensuring reliable touch input, no iframe gesture clipping, and proper viewport scaling.

---

## User Review Required

> [!IMPORTANT]
> **Key Design Decisions**:
> 1. **Skin Asset**: Using **`Arduboy (8).png`** (626×1004) from `html5/` for the mobile handheld skin.
> 2. **Auto-Detection Strategy**:
>    - By default, automatically detects mobile devices using User-Agent (`/Android|iPhone|iPad|iPod|Mobile/i`), touch capability (`maxTouchPoints > 0`), and viewport/orientation (`portrait` or `width <= 768px`).
>    - Allows manual URL override via `?skin=bare` (or `?mode=desktop`) and `?skin=arduboy` (or `?mode=mobile`) for debugging and user preference.
> 3. **itch.io Mobile Iframe Interactivity**:
>    - In mobile iframes on itch.io, browsers restrict touch gestures and scroll the page when swiping the D-pad.
>    - Following GB Studio's export pattern: if detected on a mobile device inside an iframe (`window !== window.top`), the game presents a lightweight retro "Tap to Play Fullscreen" overlay. Tapping it calls `window.open(standaloneUrl, '_blank')`, launching the full Arduboy skin in a dedicated mobile browser tab.
> 4. **Touch Input Latching**:
>    - The Arduboy AVR core polls buttons at 60 FPS (~16.6ms intervals). To prevent fast taps from disappearing between frames, button presses will ensure a minimum latch duration of ~50ms (3 frames).

---

## Proposed Changes

### 1. Web Assets & Emulator Configuration

#### [NEW] [dist/web/Arduboy (8).png](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/Arduboy%20%288%29.png)
- Copy `html5/Arduboy (8).png` to `dist/web/Arduboy (8).png`.

#### [MODIFY] [dist/web/app.js](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/app.js) and [html5/app.js](file:///home/dorito/Developer/arduboy/supermassive-whitehole/html5/app.js)
- **Module 41 (`Arduboy.js` skin definition)**:
  - Update skin definition to use `"Arduboy (8).png"`, with dimensions `626 x 1004`.
  - Calibrate hitboxes and positions to match `Arduboy (8).png`:
    - Screen: `top: "8.2%"`, `left: "17.1%"`, `width: "65.8%"`, `height: "21.5%"` (2:1 OLED aspect ratio, aligned with bezel).
    - D-pad Up: `top: "49.2%"`, `left: "15.8%"`, `width: "19.0%"`, `height: "8.2%"`
    - D-pad Down: `top: "66.6%"`, `left: "15.8%"`, `width: "19.0%"`, `height: "7.8%"`
    - D-pad Left: `top: "55.9%"`, `left: "3.2%"`, `width: "20.4%"`, `height: "11.9%"`
    - D-pad Right: `top: "55.9%"`, `left: "30.7%"`, `width: "12.5%"`, `height: "11.9%"`
    - Button A: `top: "55.7%"`, `left: "62.1%"`, `width: "16.0%"`, `height: "12.0%"`
    - Button B: `top: "52.9%"`, `left: "78.3%"`, `width: "16.1%"`, `height: "12.5%"`
- **Module 19 (`BTN.js`)**:
  - Add `pointerdown` / `pointerup` / `touchstart` / `touchend` / `touchcancel` handlers with `event.preventDefault()` to prevent scrolling and duplicate mouse events.
  - Implement minimum hold latch (~50ms) so that rapid touch taps are guaranteed to be polled by the Arduboy 60 FPS loop.
- **Module 7 (`App.js`) & Module 51 (`boot`)**:
  - Auto-detector function `detectIsMobile()`:
    - Checks URL parameter `?skin=...` or `?mode=...` first.
    - If unspecified: checks user agent (`/Android|iPhone|iPad|iPod|Mobi/i`) and touch + screen dimensions.
    - Selects `skins.Arduboy` on mobile and `skins.BareFit` on desktop.
  - Fix `initIFrame()`:
    - On **desktop iframe**: do NOT block keyboard events (`keydown`/`keyup`), allowing full gameplay inside the itch.io embed.
    - On **mobile iframe**: show mobile launch overlay / handle tap to open the full-screen emulator in the browser.

#### [MODIFY] [dist/web/style.css](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/style.css) and [html5/style.css](file:///home/dorito/Developer/arduboy/supermassive-whitehole/html5/style.css)
- Add touch control optimization: `touch-action: none;` and `-webkit-touch-callout: none;` on `#simContainer` and `btn`.
- Add active visual feedback for on-screen buttons when pressed (subtle translucent highlight).
- Style the mobile itch.io iframe "Tap to Play" overlay.

#### [MODIFY] [dist/web/index.html](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/index.html) and [html5/index.html](file:///home/dorito/Developer/arduboy/supermassive-whitehole/html5/index.html)
- Ensure responsive meta tags and container structure.

---

### 2. Build Pipeline & Packaging

#### [MODIFY] [scripts/build_and_run.sh](file:///home/dorito/Developer/arduboy/supermassive-whitehole/scripts/build_and_run.sh)
- Synchronize `Arduboy (8).png` into `dist/web/` automatically.
- Support `--mobile` option (convenience for `--skin arduboy`).

#### [MODIFY] [scripts/package_dist.sh](file:///home/dorito/Developer/arduboy/supermassive-whitehole/scripts/package_dist.sh)
- Ensure distribution zip includes `Arduboy (8).png` and all updated emulator assets.

#### [MODIFY] [docs/TODO.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/TODO.md) & [docs/distribution.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/distribution.md)
- Update TODO item status and document the mobile detection and itch.io iframe workflow.

---

## Verification Plan

### Automated / Browser Verification
1. **Desktop Mode Verification**:
   - Open `http://localhost:8000/` without parameters.
   - Verify emulator defaults to pure 2:1 OLED screen (`BareFit`) with no skin or emulated buttons.
   - Verify keyboard keys (`ArrowUp`, `ArrowDown`, `ArrowLeft`, `ArrowRight`, `KeyZ` / `KeyA`, `KeyX` / `KeyB`) control the game and start gameplay.
2. **Mobile Skin Verification**:
   - Open `http://localhost:8000/?skin=arduboy` or with mobile user-agent / viewport emulation.
   - Verify `Arduboy (8).png` renders centered and crisp.
   - Verify button coordinates align accurately with D-pad and A/B buttons.
   - Simulate touch/mouse press on Button A: verify game starts from title screen to gameplay.
   - Simulate touch/mouse press on D-pad: verify player movements.
3. **Iframe Simulation**:
   - Test in an iframe wrapper (mimicking itch.io) on desktop: verify game plays directly with keyboard.
   - Test in an iframe wrapper on simulated mobile: verify "Tap to Play" overlay prompts and launches standalone view.
4. **Build & Package Verification**:
   - Run `./build.sh --package --build-only` to ensure clean compilation and verify zip contents.
