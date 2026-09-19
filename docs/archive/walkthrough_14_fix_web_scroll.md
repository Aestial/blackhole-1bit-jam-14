# Walkthrough: HTML5 Web Emulator Cursor Hiding & Scroll Prevention

## Summary of Accomplishments

We implemented **GB Studio-style behavior** for the ProjectABE HTML5 web emulator on itch.io, resolving the issue where pressing arrow keys scrolled the container webpage and keeping the OS mouse pointer from cluttering the 1-bit display:

1. **Host Container Scroll Lock (GB Studio Behavior)**:
   - Fixed the issue in [dist/web/app.js](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/app.js) and [html5/app.js](file:///home/dorito/Developer/arduboy/supermassive-whitehole/html5/app.js) where active-low button pins caused `inputDown()` to return `false`, preventing `evt.preventDefault()` from executing.
   - Patched `BTN.js` (Module 19) to return `true` on button presses/releases.
   - Added explicit prevention in `initKeyboard()` (Module 7) for `ArrowUp`, `ArrowDown`, `ArrowLeft`, `ArrowRight`, `Space`, `PageUp`, `PageDown`, `Home`, and `End`.
   - Added an early capture-phase window event listener in [dist/web/index.html](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/index.html) and [html5/index.html](file:///home/dorito/Developer/arduboy/supermassive-whitehole/html5/index.html) matching the GB Studio web export pattern to intercept scrolling navigation keys before the browser scrolls the parent page.

2. **Cursor Hiding over Game Viewport**:
   - Updated [dist/web/style.css](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/style.css) and [html5/style.css](file:///home/dorito/Developer/arduboy/supermassive-whitehole/html5/style.css) to set `cursor: none !important;` on `body`, `#simBox`, `#simContainer`, and `canvas#screen`.
   - Preserved visible and functional cursors (`auto`, `pointer`, `text`) for `#ideContainer` so that the developer debugger remains fully usable if opened.
   - Added `overflow: hidden;` to `html, body` to eliminate internal scrollbars.

3. **Auto-Focus Latching**:
   - Configured `pointerdown`, `click`, and `focus` handlers in `index.html` and `initIFrame()` so that clicking into the game iframe immediately acquires window focus and marks the game as focused (`.game-focused`).

4. **Enhanced itch.io Simulation Framework**:
   - Rebuilt [dist/web/test_iframe.html](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/test_iframe.html) with a realistic itch.io container layout (navbar, header, controls grid, lore card, and community comments extending to 2200px height).
   - Added live scroll diagnostics (`valScrollY`, `valLastKey`, `valMode`, status badge) to verify real-time scroll immunity.

5. **Updated Documentation & Release Package**:
   - Checked off the task in [docs/TODO.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/TODO.md).
   - Documented the GB Studio scroll lock, focus latching, and cursor hiding in [docs/distribution.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/distribution.md).
   - Ran `./build.sh --package --build-only` to rebuild [dist/supermassive-whitehole-web.zip](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/supermassive-whitehole-web.zip) (194 KB) and its mirrors.

---

## Browser Verification Results

Full interactive verification was executed using the browser subagent in [dist/web/test_iframe.html](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/test_iframe.html):

| Metric | Pre-Test | Post-Keypresses | Status |
|---|---|---|---|
| **Container Scroll Y** | 80 px | 80 px | **Unchanged (0px scroll)** |
| **Canvas Cursor** | `none` | `none` | **Hidden** |
| **Key Handling** | - | `ArrowDown`, `ArrowUp`, `ArrowLeft`, `ArrowRight`, `Space`, `KeyZ` | **Captured without bubbling** |
| **Gameplay State** | Title screen | Game active / Game over screen | **Responsive** |

### Screenshots

| itch.io Container Harness | Game in Viewport & Focused |
|---|---|
| ![Initial Page](/home/dorito/.gemini/antigravity-ide/brain/961e2edd-3b05-4ef4-a58e-5cc44493ad61/initial_iframe_page_1789787691491.png) | ![Game Focused](/home/dorito/.gemini/antigravity-ide/brain/961e2edd-3b05-4ef4-a58e-5cc44493ad61/game_in_viewport_1789787993118.png) |

| Gameplay Active (Title to Game) | Mobile Handheld Embed Layout |
|---|---|
| ![Active Game](/home/dorito/.gemini/antigravity-ide/brain/961e2edd-3b05-4ef4-a58e-5cc44493ad61/gameplay_started_1789788219890.png) | ![Mobile Embed](/home/dorito/.gemini/antigravity-ide/brain/961e2edd-3b05-4ef4-a58e-5cc44493ad61/mobile_embed_view_1789788328959.png) |

---

## Changed Files

- [dist/web/index.html](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/index.html) & [html5/index.html](file:///home/dorito/Developer/arduboy/supermassive-whitehole/html5/index.html):
  - Early capture-phase scroll prevention for arrow keys and space.
  - Auto-focus latching on `pointerdown` and `click`.
  - Cache buster updated to `v=0.3.4`.
- [dist/web/style.css](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/style.css) & [html5/style.css](file:///home/dorito/Developer/arduboy/supermassive-whitehole/html5/style.css):
  - `cursor: none !important` on canvas and simulation box.
  - `overflow: hidden` on html/body.
  - Protected IDE cursors (`auto`, `pointer`, `text`).
- [dist/web/app.js](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/app.js) & [html5/app.js](file:///home/dorito/Developer/arduboy/supermassive-whitehole/html5/app.js):
  - `BTN.js`: Return `true` on input handling.
  - `App.js`: Ensure `preventDefault()` and `stopPropagation()` execute on navigation keys.
  - `initIFrame`: Call `window.focus()`.
- [dist/web/test_iframe.html](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/test_iframe.html):
  - Realistic scrollable container testing harness with live scroll diagnostics.
- [docs/TODO.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/TODO.md):
  - Marked web emulator scroll fix and cursor hiding as complete.
- [docs/distribution.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/distribution.md):
  - Documented GB Studio behavior, scroll lock, and cursor hiding.
- [dist/supermassive-whitehole-web.zip](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/supermassive-whitehole-web.zip):
  - Re-generated release zip (194 KB) ready for upload to itch.io.
