# Implementation Plan: Complete M3, Finish M4, and Implement M5

This plan addresses the next tasks from [docs/TODO.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/TODO.md): verifying and completing Milestone M3, finishing Milestone M4 (better 2D spacetime grid warp effect and UI screen polish), and implementing Milestone M5 (Arduboy audio investigation & implementation, sound effects, ambient cosmic tune, visual polish), along with updating all documentation and source code comments to reflect milestone completion.

---

## User Review Required

> [!NOTE]
> **Audio System Architecture**: The Arduboy hardware features dual piezo buzzer pins driven by ATmega32u4 timers, and the local toolchain already has `ArduboyTones` (1.0.3) installed. Furthermore, the ProjectABE web emulator in `html5/` simulates these audio pins through WebAudio `AudioContext`. We will implement audio cleanly through a new platform-independent `HalAudio` interface in `src/hal/audio.h` and concrete `ArduboyAudio` in `src/platform/arduboy/arduboy_audio.h`, keeping game logic 100% portable.

> [!TIP]
> **Memory Headroom**: Current build uses 24,446 bytes of Flash (85%) and 1,603 bytes of SRAM (62%). The additions for M3 absorption, M4 2D grid warping & UI polish, and M5 audio & tones fit comfortably within the remaining 4,226 bytes of Flash and 957 bytes of SRAM.

---

## Proposed Changes

### Milestone M3: Verification & Entity Absorption Completion

1. **Entity Absorption Growth** ([docs/gdd.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/gdd.md#L85))
   - When food is absorbed into the whitehole core (< `BH_ABSORB_RADIUS`), increment `world.bhMass` slightly (`+BH_FOOD_ABSORB_MASS_INC = 0.02`).
   - When money/collectibles are absorbed, increment `world.bhCharge` slightly (`+BH_MONEY_ABSORB_CHARGE_INC = 1px`).
   - Add constants to [config.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/config.h).

---

### Milestone M4: Better Warp Effect & UI Polish

#### 1. Better Spacetime Grid Warp Effect ([game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp#L408-L485))
- **Full 2D Gravitational Lensing**:
  - Replace 1D X-only distortion with radial 2D distortion: both `shiftX` and `shiftY` pull inward toward the whitehole center `(bhSX, bhSY)` with quadratic distance falloff.
  - **Horizontal Depth Lines**: Distort both `cx` and `cy`, causing lines to curve naturally up and down toward the singularity.
  - **Vertical Perspective Rays**: Instead of distorting only the top and bottom endpoints (which left rays as straight diagonal segments), divide each vertical ray into 4 segments across the screen height and distort each vertex in 2D.
  - This creates an organic 2D spacetime curvature / gravity funnel effect.

#### 2. Polished Title Screen ([game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp#L109-L125))
- Add attract-mode blinking cadence for "Press A" (32 frames on, 16 frames off).
- Add gentle cosmic sparkle / floating stardust around the swirling title whitehole.

#### 3. Polished Game Over Screen ([game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp#L372-L402))
- Wrap the screen in a decorative double-line arcade frame.
- Display "GAME OVER" centered with bold styling.
- New High Score detection: If `score >= highScore && score > 0`, display a celebratory flashing `★ NEW RECORD! ★` badge.
- Formatted statistics panel:
  - `SCORE:      <score>`
  - `RECORD:     <highScore>`
  - `SURVIVED:   <seconds>s`
- Clean button prompt: `[A] MENU    [B] RETRY` with blinking indicator when cooldown finishes.

#### 4. Polished HUD Screen ([game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp#L667-L717))
- Expand score badge dynamically or ensure generous width (`HUD_BG_W = 46`) so 5-digit scores and combo multipliers never clip.
- Add active status badges:
  - Coffee Turbo: `[TURBO]` badge with animated exclamation.
  - Food Slow: `[SLOW]` indicator with blinking warning.
- Stamina Bar polish: crisp border, end caps, and smooth flashing when critical.

---

### Milestone M5: Audio HAL, Sound Effects, Ambient Music & Visual Polish

#### 1. Hardware Abstraction Layer for Audio
- #### [NEW] [src/hal/audio.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/hal/audio.h)
  - Abstract HAL interface for sound effects, tones, music, and mute toggle.
- #### [NEW] [src/platform/arduboy/arduboy_audio.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/platform/arduboy/arduboy_audio.h)
  - Concrete implementation using `ArduboyTones` and `Arduboy2Audio`.
- #### [MODIFY] [src/hal/hal_types.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/hal/hal_types.h)
  - Register `ArduboyAudio` as `HalAudio`.
- #### [MODIFY] [supermassive-whitehole.ino](file:///home/dorito/Developer/arduboy/supermassive-whitehole/supermassive-whitehole.ino)
  - Instantiate `ArduboyAudio audio` and pass to `game.init(storage, audio)`.

#### 2. Sound Effects & Music Scores
- #### [NEW] [src/game/audio_scores.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/audio_scores.h)
  - Define PROGMEM tone sequences using `ArduboyTones` format:
    - `SOUND_DIAMOND`: High crisp double chime (B5 -> E6)
    - `SOUND_BILLS`: Cash arpeggio (G5 -> C6 -> G6)
    - `SOUND_BOOST`: Turbo acceleration fanfare
    - `SOUND_FOOD_SLOW`: Low descending crunch (F#3 -> D3)
    - `SOUND_ABSORB`: Deep rumble when whitehole consumes food/money
    - `SOUND_GAMEOVER`: Dramatic descending game over melody
    - `SOUND_CLICK`: Snappy menu blip
    - `SOUND_AMBIENT_LOOP`: Minimalist rhythmic cosmic pulse / bass groove that loops during gameplay.

#### 3. Sound Integration in Game Logic
- #### [MODIFY] [src/game/game.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.h) & [src/game/game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp)
  - Store `HalAudio* audioRef`.
  - Trigger SFX on item pickup, food hazard collision, boost activation, entity absorption, game over, and button presses.
  - Trigger background ambient loop in `STATE_PLAYING`.

#### 4. Visual Polish & Outline Setting
- Confirm `PLAYER_OUTLINE_MODE` is `SPRITE_OUTLINE_NONE` and mark off TODO line 29.

---

### Documentation & Milestone Status Updates

#### [MODIFY] [docs/TODO.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/TODO.md)
- Check off:
  - `- [x] Verify milestone M3 completion because agents didn't update architecture.md, gdd.md and TODO.md after implementation.`
  - `- [x] Finish with milestone M4: better warp effect and polish all UI screens (title, game over and HUD).`
  - `- [x] Continue with milestone M5.`
  - `- [x] Investigate about Arduboy audio output and music/sfx capabilities.`
  - `- [x] Add sound effects.`
  - `- [x] Add ambient music.`
  - `- [x] Remove outline from the player sprite.`

#### [MODIFY] [docs/gdd.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/gdd.md)
- Update Milestones table:
  - M3: `✅ Complete`
  - M4: `✅ Complete`
  - M5: `✅ Complete`
- Note the addition of entity absorption mass/charge effects and audio features.

#### [MODIFY] [docs/architecture.md](file:///home/dorito/Developer/arduboy/supermassive-whitehole/docs/architecture.md)
- Update architecture diagram to include `HalAudio` / `ArduboyAudio`.
- Update Milestones table:
  - M3: `✅ Complete`
  - M4: `✅ Complete`
  - M5: `✅ Complete`

#### [MODIFY] Source Comments in [src/game/game.cpp](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.cpp), [src/game/game.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/game.h), [src/game/world.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/world.h), [src/game/physics.h](file:///home/dorito/Developer/arduboy/supermassive-whitehole/src/game/physics.h)
- Replace stale M0/M1 TODO comments with accurate implementation status notes.

---

## Verification Plan

### Automated Compilation & Memory Sizing
- Run `./build.sh --build-only` to ensure:
  - Arduino sketch compiles cleanly with 0 warnings/errors.
  - Flash memory usage remains under 28,672 bytes.
  - Dynamic memory (SRAM) usage remains under 2,000 bytes (~560+ bytes stack headroom).
  - Distribution `.hex` and web artifacts are properly synchronized.

### Emulation Verification via Browser Subagent
- Launch `./build.sh` (or web server at `localhost:8000`) and test in browser:
  - **Title Screen**: Animated whitehole, high score, blinking "Press A", stardust sparkles.
  - **Playing State**:
    - Smooth 60 FPS gameplay.
    - 2D spacetime grid curvature warping toward whitehole as it moves.
    - Sound effects play on item pickup (coins/diamonds), coffee boost, and food hits.
    - Accretion particles swirling around whitehole.
    - HUD displays score, combo streaks, boost/slow badges, and stamina bar.
  - **Game Over**:
    - Dramatic game over jingle.
    - Polished arcade frame, score, record, survival time, retry/menu prompts.
