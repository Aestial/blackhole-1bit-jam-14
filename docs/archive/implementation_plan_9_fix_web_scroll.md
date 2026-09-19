# Implementation Plan: Hide Cursor & Disable Container Scrolling in HTML5 Emulator

Implement GB Studio-style behavior for the ProjectABE HTML5 web emulator on itch.io:
1. **Disable Scrolling on Container Webpage**: Prevent the host webpage from scrolling when pressing arrow keys or space inside the game iframe.
2. **Hide Cursor on Focus / Game Canvas**: Hide the mouse pointer when focusing on or hovering over the game viewport to eliminate visual distraction on the 1-bit display.
3. **Update Documentation & Distribution**: Update [docs/TODO.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/TODO.md), [docs/distribution.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/distribution.md), and rebuild the release bundle via [scripts/package_dist.sh](file:///home/dorito/Developer/arduboy/supermassive-whitehole/scripts/package_dist.sh).

---

## Root Cause Analysis

1. **Host Page Scrolling on Arrow Keys**:
   - In `app.js` Module 7 (`App.js`), `initKeyboard()` checks `var ret = this.inputDown(code); if (ret === true) { evt.preventDefault(); evt.stopPropagation(); }`.
   - In Module 19 (`BTN.js`), button press handlers return `this.on.value = this.active`. Because Arduboy buttons are configured with `active="low"` (active low logic), `this.active` is `false`.
   - Therefore, `inputDown` returns `false`, `ret === true` evaluates to `false`, and `evt.preventDefault()` is **never called** for `ArrowUp`, `ArrowDown`, `ArrowLeft`, `ArrowRight`!
   - In addition, keys like `Space`, `PageUp`, `PageDown`, `Home`, and `End` are not mapped to game buttons and had no `preventDefault()`.
   - Without `e.preventDefault()`, the browser treats arrow keys inside the iframe as unhandled navigation actions and executes default scrolling on the enclosing container webpage (itch.io).

2. **Visible Cursor**:
   - Neither `style.css` nor `index.html` set `cursor: none` on `canvas#screen`, `#simContainer`, or `body`.
   - As a result, when playing the game on desktop, the OS mouse pointer remains hovering directly over the retro 128×64 pixel canvas.

---

## Proposed Changes

### 1. Web Emulator Core & Styling

#### [MODIFY] [dist/web/index.html](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/index.html) & [html5/index.html](file:///home/dorito/Developer/arduboy/supermassive-whitehole/html5/index.html)
- Add early capture-phase window listener matching GB Studio web export to prevent default scrolling on `ArrowUp`, `ArrowDown`, `ArrowLeft`, `ArrowRight`, `Space`, `PageUp`, `PageDown`, `Home`, `End` (both by `e.code` and `e.key`/`e.keyCode`).
- Add auto-focus handlers on `pointerdown` and `click` to guarantee iframe window focus when user interacts with the game.
- Bump cache buster query parameter from `v=0.3.3` to `v=0.3.4`.

#### [MODIFY] [dist/web/style.css](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/style.css) & [html5/style.css](file:///home/dorito/Developer/arduboy/supermassive-whitehole/html5/style.css)
- Add `overflow: hidden` to `html, body` to prevent any internal scrollbar emergence.
- Set `cursor: none` on `body`, `#simBox`, `#simContainer`, and `canvas#screen`.
- Explicitly restore `cursor: auto`, `cursor: pointer`, and `cursor: text` on `#ideContainer` so that if the debugger is opened, controls remain usable.

#### [MODIFY] [dist/web/app.js](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/app.js) & [html5/app.js](file:///home/dorito/Developer/arduboy/supermassive-whitehole/html5/app.js)
- Update Module 19 (`BTN.js`) to ensure `onPress` and `onRelease` always return `true` when handling bound keys.
- Update Module 7 (`App.js`) in `initKeyboard()` to call `evt.preventDefault()` and `evt.stopPropagation()` whenever a navigation or game key is pressed.

---

### 2. Testing Framework & Packaging

#### [MODIFY] [dist/web/test_iframe.html](file:///home/dorito/Developer/arduboy/supermassive-whitehole/dist/web/test_iframe.html)
- Enhance test harness with a realistic scrollable itch.io container page (header banner, title, scrollable description and comments above and below the iframe).
- Add live scroll-position tracker and event log to verify that pressing arrow keys inside the focused game iframe does not scroll the parent container page by even a single pixel.

#### [MODIFY] [scripts/package_dist.sh](file:///home/dorito/Developer/arduboy/supermassive-whitehole/scripts/package_dist.sh)
- Re-package `dist/supermassive-whitehole-web.zip`, `dist/whitehole-web.zip`, and `dist/blackhole-web.zip`.

---

### 3. Documentation

#### [MODIFY] [docs/TODO.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/TODO.md)
- Check off task: `Fix HTML5 web emulator scroll when using arrow keys, getting game canvas out of sight and hide cursor when focusing/playing.`

#### [MODIFY] [docs/distribution.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/distribution.md)
- Document the itch.io embed scroll prevention, focus latching, and cursor-hiding behavior matching GB Studio.

---

## Verification Plan

### Automated / Browser Subagent Verification
1. **Start Local Test Server**:
   - Run `python3 -m http.server 8000 --directory dist/web`.
2. **Browser Subagent Test**:
   - Open `http://localhost:8000/test_iframe.html`.
   - Scroll down the container page to position the iframe in view.
   - Click inside the iframe to focus.
   - Verify cursor is hidden over the game canvas.
   - Send `ArrowDown`, `ArrowUp`, `ArrowLeft`, `ArrowRight`, `Space`, `KeyZ` keystrokes into the iframe.
   - Verify the parent container page does not scroll (scroll offset unchanged).
   - Verify game receives inputs and reacts normally.
   - Test in both desktop mode and mobile mode (`?mode=mobile`).
3. **Distribution Package Validation**:
   - Run `./build.sh --package --build-only`.
   - Verify zip archive contents and integrity.
