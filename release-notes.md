# pico-gamate 3.0.8 --- Release Notes

Version 3.0.8 is a maintenance release focused on MURMULATOR 2 audio and
PSRAM correctness, persistent Demo/TV-SOFT settings, safer cartridge
loading, Demo-mode recovery, and cartridge-browser navigation.

## Highlights

-   Fixed MURMULATOR 2 PWM routing: stereo PWM now uses GP10/GP11 and no
    longer conflicts with the RP2350A QSPI PSRAM CS1 pin on GP8.
-   Fixed hardware AY-3-8910 / TurboSound 74xx595 pin mapping so it follows
    each board's audio-pin definitions instead of fixed MURMULATOR 1 GPIOs.
-   Added persistent Demo duration and expanded the available Demo times to
    15 sec, 30 sec, 45 sec, 1 min, 2 min, 3 min, 5 min, and 10 min.
-   Added persistence for the TV-SOFT `Colors` option.
-   Updated the settings format to version 3 with strict size/version
    validation and checked configuration writes.
-   Added cartridge-load validation and Flash post-program verification.
-   Demo mode now skips cartridges that fail to load instead of stopping on
    the first failed entry.
-   Fixed stale/sticky Demo state when control returns to the cartridge
    browser through a non-Demo path.
-   Added keyboard `PageUp` / `PageDown` navigation by half a visible page.
-   Updated the default CMake selection to MURMULATOR 2 with HDMI enabled.

## MURMULATOR 2 PWM / PSRAM fix

The MURMULATOR 2 board definition now sets `AUDIO_PWM_PIN` to GP10.
Because the PWM backend uses an even/odd stereo pair, this gives GP10/GP11.

Previously the base pin was GP9, which made the PWM pair GP8/GP9. On
RP2350A-based MURMULATOR 2 boards GP8 is QSPI PSRAM CS1, so enabling PWM
could reconfigure the PSRAM chip-select pin and break cartridge access.

The audio default configuration now explicitly uses `AUDIO_PWM_PIN` and
`AUDIO_PWM_PIN + 1` for PWM builds, while I2S keeps using
`AUDIO_DATA_PIN` / `AUDIO_CLOCK_PIN`.

## Hardware AY pin mapping

The hardware AY-3-8910 / TurboSound 74xx595 interface no longer hard-codes
GPIO 26/28 for its clock/latch and data signals. It derives those signals
from the board audio-pin definitions instead.

This preserves the existing MURMULATOR 1 mapping while allowing
MURMULATOR 2 to use its own audio-pin layout correctly.

## Persistent settings format v3

The settings structure is updated to version 3.

The following settings added since 3.0.7 are persistent:

-   Demo game duration
-   TV-SOFT `Colors` mode

Configuration loading accepts only the exact current structure size and
version 3. Older shortened structures are no longer accepted as the
current configuration.

Configuration saving now checks both the number of bytes written and the
result of `f_close()`. A configuration write is successful only when the
full settings block is written and the file closes successfully.

Because of the structure/version change, existing version-1/version-2
configuration files are not loaded as current settings. Current defaults
are used until settings are saved again.

## Expanded Demo durations

Demo mode now offers:

-   15 seconds
-   30 seconds
-   45 seconds
-   1 minute
-   2 minutes
-   3 minutes
-   5 minutes
-   10 minutes

The selected value is stored in the version-3 configuration.

## Cartridge-load validation

A cartridge is no longer treated as successfully loaded merely because a
load attempt was started. The loader now checks the relevant file-system
results and the total number of bytes read.

Load failures are reported for cases including:

-   missing or empty cartridge files;
-   cartridges that exceed the supported size;
-   cartridges too large for available PSRAM;
-   incomplete/read-error loads;
-   Flash verification failures.

`rom_size` and the active cartridge filename are updated only after a
successful load, so a failed manual selection does not silently turn into
the previously loaded cartridge.

## Flash verification

Flash-backed cartridge loading still compares each sector first and skips
sectors whose contents already match the requested data.

After a changed sector is erased and programmed, the programmed XIP data
is compared with the source buffer. A mismatch aborts the load and shows:

```text
ERROR: Flash verify failed!
```

## Demo-mode load recovery

During Demo mode, a cartridge that fails to load is skipped and the next
alphabetical cartridge is tried. Demo-mode load errors use a shorter delay
so automatic cycling can continue; manual load errors remain visible
longer.

## Demo state reset on browser entry

The cartridge browser is now always entered as a non-Demo state. Before a
real browser entry, the firmware clears:

```text
demo_active
demo_requested
demo_advance_pending
```

and hides the Demo title overlay.

The same cleanup is performed on the generic path returning from emulation
to the browser. Normal timed Demo cartridge-to-cartridge transitions
continue directly above that path, so automatic Demo cycling is preserved.

## Cartridge-browser PageUp / PageDown

Keyboard `PageUp` and `PageDown` now move the selected cartridge by half
of the visible page.

The cursor moves first. The viewport is kept in place while the target
remains visible, and scrolls only when the new selection would move beyond
the current visible range.

## TV-SOFT Colors persistence

The TV-SOFT `Colors` menu option now uses the persistent settings
structure instead of a separate runtime-only variable, so the selected
color/monochrome state survives restart together with the other emulator
settings.

## Build defaults and release matrix

The checked-in default CMake configuration now selects MURMULATOR 2 and
HDMI, and the project version is 3.0.8.

The release build matrix remains the same as 3.0.7:

-   RP2350 / ARM Cortex-M33 targets;
-   MURMULATOR 1.x;
-   MURMULATOR 2.0;
-   Olimex RP2040-PICO-PC board family with RP2350/Pico 2;
-   RP2350-PiZero;
-   VGA, HDMI, or TV-SOFT video;
-   PWM, I2S, I2S-CS4334, or hardware AY-3-8910 audio.

This gives 48 standard release configurations.

## Firmware file names

Examples for this release:

```text
m1p2-gamate-VGA-PWM-3.0.8.uf2
m2p2-gamate-HDMI-I2S-3.0.8.uf2
PCp2-gamate-VGA-AY-3-8910-3.0.8.uf2
z0p2-gamate-TV-SOFT-I2S-3.0.8.uf2
```

MURMULATOR Ultimate v2.x continues to use the MURMULATOR 1.x pinout and
therefore uses the `m1` firmware prefix.
