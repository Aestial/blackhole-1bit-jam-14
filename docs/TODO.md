# TODOs

## Urgent (priority 1-2)

Most urgent tasks, ordered by priority (p1 > p2).

- [x] Make a proper game page on itch.io.
- [x] Submit the game to the 1-Bit Jam 14.
- [x] Fix sprites transparency to no transparency and add outline, except the whitehole.
- [x] Add player animation for 8 directions, use placeholders for now (current and mirrored current).
- [x] Add method for detecting user's device: desktop or mobile to display different screens in itch.io's game iframe. Just like the GB Studio HTML export. 
- [x] Fix player controller input for movement, still feels like a car (accelerate, brake) -> Goal: GTA walking controls with running and sprinting.
- [x] Implement stamina system for movement. Stamina depletes when running and regenerates when walking or idle.
- [x] Implement stamina bar which fills from bottom to top.
- [x] Whitehole gravity effects: pull entities (including player).
- [x] Whitehole visual effects: grid distortion/warp effect and accretion particles.
- [x] Fix HTML5 web emulator scroll when using arrow keys, getting game canvas out of sight and hide cursor when focusing/playing (GB Studio behavior).
- [ ] Verify milestone M3 completion because agents didn't update architecture.md, gdd.md and TODO.md after implementation.
- [ ] Finish with milestone M4: better warp effect and polish all UI screens (title, game over and HUD).
- [ ] Continue with milestone M5.
- [ ] Investigate about Arduboy audio output and music/sfx capabilities.
- [ ] Add sound effects.
- [ ] Add ambient music.
- [ ] Finish with milestone M6.

## Desired for the jam (priority 3-4)

- [ ] Create a dev log of what I learned during the jam and such.
- [ ] Remove outline from the player sprite.

## After the jam

- [ ] Upstream web emulator adjustments (responsive device auto-detection, touch latching, direct handheld mobile skin) to the source ProjectABE emulator repository (felipemanga/ProjectABE or its most popular/recent fork).
- [ ] Port to another handheld mini console, which supports C++.
- [ ] Port to TIC-80 and possibly other fantasy console, which may not support C++ but has Lua.