# pico-gamate 3.1.2 --- Release Notes

Version **3.1.2** adds per-cartridge input recording and playback, integrates
recordings with quick-save/quick-load, and improves file and reset handling.
The firmware retains the existing RP2350 board/video/audio release matrix.

## Record and play a cartridge

In the SD-card cartridge browser, select a `.bin` file and press:

-   **R** to load the cartridge and begin a new recording.
-   **P** to load the cartridge and replay its existing recording.
-   **Enter / Start** to load the cartridge without recording or playback.

The existing `B` command still enters Demo mode, and `P` remains mapped to
in-game button `B` while playing normally. `F10` opens the browser while a
game is running; returning without changing the cartridge resumes that game.

Recordings are saved to `/.config/gamate/replay/<rom-filename>.gmr`. Starting
a new recording for the same cartridge overwrites its earlier file. The
current `.gmr` format is **version 2**. It stores input transitions from the
keyboard and both gamepads with their emulated-frame numbers; playback checks
the recording's ROM name and size. Old version-1 recordings are not compatible.

Playback feeds the saved events to emulated game input without using live
gameplay buttons until the saved end. Recording uses the existing title
overlay to show `REC HH:MM:SS` based on emulated frames, with no `PLAY` OSD.

## Quick-save, quick-load and timeline branching

-   **Ctrl+F1** through **Ctrl+F8** save a state to the corresponding slot.
-   **Alt+F1** through **Alt+F8** load the corresponding slot.

Save-state files are now read and written under `/.config/gamate/`, not
`/GAMATE/`. Their names still include the ROM filename and the optional
slot number. Old files are **not moved automatically**; existing states
must be moved to `/.config/gamate/` to remain accessible.

A save state made while recording contains additional replay metadata:
recording session, frame number, file offset and source input state.
Loading a compatible save from **the same recording session** seeks back
to its saved point and truncates events belonging to the abandoned future.
Subsequent gameplay creates a new recording branch, while the REC indicator
remains visible. A state without compatible replay metadata cannot restore
that recording timeline.

Quick-save/load no longer remounts FatFs while a recording is open. The
recording is checkpointed before writing a quick-save, and its header and
pending events are synchronized approximately once per 60 emulated frames.
This also improves preservation of recordings across watchdog resets, though
the most recent unsynchronized events can still be lost.

## Demo mode, controls and configuration

Demo mode gains a countdown of remaining game time and an optional cartridge
name overlay. The new **Demo name OSD** setting can hide the title without
turning off the countdown. When a quick-state operation is used during Demo,
the automatic cartridge switching is cancelled; quick-saving during recording
does not cancel the recording session.

Emulator keyboard shortcuts include:

-   **F6** — randomize the current palette.
-   **F7** — randomize the palette and persist the emulator settings.
-   **F8** — write the current cartridge's palette preset (`.ini`).
-   **F9** — restore the default palette.
-   **F10** — open or return from the file browser.

The settings structure is now **version 5**. Existing version-4 configuration
files can be loaded and upgraded with the Demo-name option enabled by default.
Settings remain under `/.config/gamate/<platform>/<video>/emulator.cfg`; per-game
palette presets remain under `/.config/gamate/<rom-basename>.ini`.

## Reliability fixes

-   Removed remounts from quick-state operations to avoid invalidating an
    open `.gmr` recording.
-   Added replay checkpoints and corrected continuity across quick saves and
    compatible quick loads.
-   Improved PSRAM initialization and the QMI/watchdog-reset transition.
-   Corrected the hardware AY-3-8910/TurboSound PIO TX-FIFO handling so
    bursts of register writes do not overwrite a full FIFO.
-   Retained Demo OSD cleanup and file-browser behavior fixes, including the
    ability to resume the currently running game.

## Compatibility and release targets

The ROM directory remains `/GAMATE/`. Persistent data now has the following
layout:

```text
/.config/gamate/<rom-filename>.save             Save state
/.config/gamate/replay/<rom-filename>.gmr        Input recording
/.config/gamate/<rom-basename>.ini              Per-game palette preset
/.config/gamate/<platform>/<video>/emulator.cfg Settings
```

The release target remains RP2350/Cortex-M33. The standard release matrix
covers four board configurations, three video backends (VGA, HDMI and
TV-SOFT), and four audio backends (PWM, I2S, I2S-CS4334 and hardware
AY-3-8910), totaling **48 build configurations**. Firmware names and the
embedded program version now use **3.1.2**.

Random custom palettes and Demo/quick-state isolation introduced in earlier
3.1.x builds are retained in this release.
