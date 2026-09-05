# pico-gamate 3.0.7 --- Release Notes

This release substantially extends the display, demo, audio, and RP2350
support of pico-gamate.

## Highlights

-   Added a photographic Gamate backplane for the native-scale display
    modes.
-   Added and refined backplane support for VGA, HDMI, and TV-SOFT.
-   Added proper VGA 1:1 output and HDMI 1:2 output.
-   Added 4:3 rendering for HDMI and TV-SOFT.
-   Reworked PAL/NTSC TV-SOFT timing and rendering.
-   Added Demo mode, including automatic cartridge cycling and a
    cartridge-name overlay.
-   Added configurable Demo game duration: 30 sec, 45 sec, 1 min, 3 min,
    5 min, or 10 min.
-   Added hardware AY-3-8910 / TurboSound output support.
-   Added configurable gray-line level for the VGA/HDMI modes that
    support gray lines.
-   Added QSPI PSRAM support for RP2350/Pico 2 targets. Cartridge ROMs
    can be loaded into PSRAM when available.
-   Improved cartridge flashing: unchanged flash sectors are no longer
    erased/programmed again.
-   Fixed several hardware/emulated AY state and muting issues during
    Demo mode and ROM changes.
-   Fixed VGA line positioning and Demo overlay placement.
-   Fixed HDMI rendering/flow issues.
-   Fixed TV-SOFT 1:1 backplane geometry and 4:3 full-screen rendering.
-   TV-SOFT now saves the selected PAL/NTSC mode.
-   Fixed TV-SOFT Demo cartridge-name overlay positioning, dimensions,
    and colors.
-   Fixed I2S underruns under heavy TV-SOFT load: the DMA path holds the
    last stereo sample while waiting for fresh audio, and the audio DMA
    IRQ has priority over the time-critical software-video path.
-   Removed the unused `Instant ignition simulation` menu item.
-   Added a script for building the supported RP2350/Cortex-M33
    configuration matrix.

## Display modes

### VGA

VGA now supports the revised Gamate presentation modes, including a true
1:1 framebuffer presentation with the photographic console backplane.
Backplane geometry and LCD masking were refined so that the emulated
160×150 image is cleanly isolated from the photographed LCD contents.

Gray-line rendering and its positioning were also corrected, with a
selectable gray level where applicable.

### HDMI

HDMI received the Gamate backplane, corrected rendering flow, a 1:2
presentation based on the approved 1:1 composition, and a 4:3
full-screen mode.

The Demo cartridge-name overlay was repositioned and resized for the
HDMI output.

### TV-SOFT

The software composite-video output received extensive PAL/NTSC fixes.

The two user-visible modes are now:

-   **4:3** --- the 160×150 Gamate framebuffer fills the logical 320×240
    picture: exact 2× expansion horizontally and 150→240 scaling
    vertically.
-   **1:1** --- the original 160×150 framebuffer is displayed without
    geometric scaling inside the Gamate backplane.

PAL and NTSC use the same logical backplane composition; their
differences remain in the composite timing/signal generation.

The selected TV system is now stored in settings.

## Demo mode

Demo mode can be started from the menu and can also be controlled with
the NES-pad Demo shortcut.

The emulator automatically advances through cartridges after the
selected interval. Available durations are:

-   30 seconds
-   45 seconds
-   1 minute
-   3 minutes
-   5 minutes
-   10 minutes

The current cartridge name is shown in an overlay at the bottom of the
display. Overlay dimensions, positioning, bitmap stride, and colors were
corrected across the affected video paths.

Audio state is now reset correctly while changing cartridges or leaving
Demo mode, including hardware AY-3-8910 output.

## Audio

Hardware AY-3-8910 / TurboSound output is supported in addition to the
existing PWM and I2S paths.

The I2S DMA path was hardened for the particularly high CPU load of
TV-SOFT. If the producer misses a DMA boundary, I2S continues
transmitting the last stereo sample instead of starving the PIO stream.
The I2S DMA interrupt is given priority over the software-TV rendering
interrupt, eliminating the intermittent clicks observed under TV-SOFT
load.

## RP2350 QSPI PSRAM

RP2350/Pico 2 builds now support external QSPI PSRAM where provided by
the board.

The implementation probes the PSRAM, configures QMI timing, and can
place cartridge ROM data in PSRAM instead of repeatedly relying on flash
storage. Board-specific PSRAM chip-select definitions are provided for
the supported RP2350 targets.

## Flash handling

Cartridge programming now compares existing flash contents before
erase/program operations. Sectors that already contain the requested
data are left untouched, reducing unnecessary flash erase/program
cycles.

## File names legend

Current release packages target **RP2350 / ARM Cortex-M33 only**.

**Board prefix:**

-   `m1` --- Murmulator 1.x.
-   `m2` --- Murmulator 2.0.
-   `PC` --- Olimex RP2040-PICO-PC board family.
-   `z0` --- RP2350-PiZero.
-   `p2` --- Raspberry Pi Pico 2 / RP2350 generation.

For example:

-   `m1p2-gamate-VGA-PWM-3.0.7.uf2` --- Murmulator 1.x, VGA output, PWM
    sound.
-   `m2p2-gamate-HDMI-I2S-3.0.7.uf2` --- Murmulator 2.0, HDMI output,
    I2S sound.
-   `PCp2-gamate-VGA-AY-3-8910-3.0.7.uf2` --- Olimex RP2040-PICO-PC
    board, RP2350/Pico 2 build, VGA output, hardware AY-3-8910.
-   `z0p2-gamate-TV-SOFT-I2S-3.0.7.uf2` --- RP2350-PiZero, composite AV
    output generated by TV-SOFT, I2S sound.

> **N.B.** Murmulator Ultimate v2.x uses the Murmulator 1.x pinout, so
> its firmware uses the `m1` prefix.

**Video suffix:**

-   `-VGA` --- VGA output.
-   `-HDMI` --- HDMI output.
-   `-TV-SOFT` --- software-generated composite TV/AV output.

**Audio suffix:**

-   `-PWM` --- PWM audio; use this when no external I2S DAC is
    installed/enabled.
-   `-I2S` --- I2S audio for the standard supported TDA/PCM-style I2S
    DAC path.
-   `-I2S-CS4334` --- I2S audio configured for CS4334.
-   `-AY-3-8910` --- hardware AY-3-8910 / TurboSound output.

**File extension:**

-   `.uf2` --- firmware image for direct installation on the RP2350/Pico
    2 target.

## Build scope

The release build matrix covers the supported RP2350/Cortex-M33 targets
with:

-   VGA, HDMI, or TV-SOFT video;
-   PWM, I2S, I2S-CS4334, or hardware AY-3-8910 audio.

RP2040 (`p1`), RISC-V, `m1p2launcher`, TFT/ILI9341, legacy `TV`, and
`.m1p2` launcher packages are not part of this release matrix.
