# pico-gamate

**Bit Corporation Gamate emulator for RP2350-based MURMULATOR systems
and compatible boards.**

`pico-gamate` runs Gamate software directly on Raspberry Pi Pico 2 /
RP2350-class hardware and provides several video and audio backends
intended for MURMULATOR-style retro-computer hardware.

Current version: **3.0.8**

## Current target

The current release build targets **RP2350 / ARM Cortex-M33**.

Supported board configurations:

-   **MURMULATOR 1.x**
-   **MURMULATOR 2.0**
-   **Olimex RP2040-PICO-PC** board family with an RP2350/Pico 2 target
-   **RP2350-PiZero**

MURMULATOR Ultimate v2.x uses the MURMULATOR 1.x pinout and therefore
uses the `m1` firmware prefix.

## Features

-   Bit Corporation Gamate emulation
-   SD-card cartridge browser
-   PS/2 keyboard support
-   NES gamepad support
-   VGA output
-   HDMI output
-   software-generated PAL/NTSC composite output (**TV-SOFT**)
-   PWM audio
-   I2S audio
-   CS4334 I2S audio configuration
-   hardware AY-3-8910 / TurboSound output
-   QSPI PSRAM support on supported RP2350 boards
-   Demo mode with automatic cartridge switching
-   photographic Gamate backplane in native-scale display modes
-   configurable display scaling and gray-line effects on supported
    digital/RGB modes
-   persistent emulator settings

## Video output

### VGA

VGA is the standard RGB output for MURMULATOR hardware.

The emulator provides several presentation modes, including a
native-scale Gamate framebuffer inside a photographic console backplane.
Gray-line simulation is available in the applicable scaled modes.

### HDMI

HDMI output is supported through the RP2350 video implementation.

The current renderer includes native/aspect presentation using the
Gamate backplane as well as a full-screen 4:3 presentation. Gray-line
simulation is available in the applicable mode.

### TV-SOFT

`TV-SOFT` generates composite PAL/NTSC video in software and is distinct
from the old `TV` backend.

Two display modes are available:

-   **4:3** --- the 160×150 Gamate image is expanded to the full logical
    320×240 picture.
-   **1:1** --- the original 160×150 framebuffer is displayed without
    geometric scaling inside the Gamate backplane.

The selected PAL/NTSC standard and the TV-SOFT `Colors` option are stored in the emulator settings.

## Audio

The release can be built with one of four audio configurations:

-   **PWM** --- direct PWM audio; no external I2S DAC is required.
-   **I2S** --- external I2S DAC output.
-   **I2S-CS4334** --- I2S configured for CS4334.
-   **AY-3-8910** --- hardware AY-3-8910 / TurboSound output.

The I2S implementation includes underrun handling intended to keep the
audio stream stable even under the relatively high interrupt load of
TV-SOFT.

On MURMULATOR 2, PWM stereo uses GP10/GP11. This avoids GP8, which is the
RP2350A QSPI PSRAM CS1 pin. The PWM backend now derives its output pins
from `AUDIO_PWM_PIN`, while I2S continues to use the board-specific
`AUDIO_DATA_PIN` / `AUDIO_CLOCK_PIN` pair.

Hardware AY-3-8910 / TurboSound output now derives the 74xx595 data and
clock/latch wiring from the board audio-pin definitions instead of fixed
MURMULATOR 1 GPIO numbers, so the same code follows the MURMULATOR 2
pinout correctly.

## Cartridge storage

Cartridges are selected from the SD-card browser.

On supported RP2350 boards with QSPI PSRAM, cartridge ROM data can be
placed in PSRAM. The firmware detects and initializes the external PSRAM
where the board configuration provides it.

Flash programming avoids erase/program operations for sectors whose
contents are already unchanged, reducing unnecessary flash writes. Changed
sectors are verified after programming. A missing, empty, incomplete, too
large, or unsuccessfully verified cartridge is reported as a load failure
instead of being accepted as the current ROM.

## Demo mode

Demo mode automatically loads cartridges in sequence and advances to the
next game after a configurable interval. The selected duration is stored
in the emulator settings.

Available game durations:

-   15 seconds
-   30 seconds
-   45 seconds
-   1 minute
-   2 minutes
-   3 minutes
-   5 minutes
-   10 minutes

While a cartridge is started in Demo mode, its name is displayed in a
strip along the bottom of the screen.

If a cartridge cannot be loaded, Demo mode briefly shows the error, skips
that cartridge, and continues with the next one. Entering the cartridge
browser clears the active Demo state so a non-Demo return to the browser
cannot leave Demo mode latched.

Demo mode can be started from the emulator menu.

## Cartridge browser navigation

Keyboard `PageUp` and `PageDown` move the current selection by half of the
visible page. The cursor moves first; the list viewport scrolls only when
the new selection would otherwise fall outside the visible area.

## Configuration format

The current settings format is version 3. Demo duration and the TV-SOFT
`Colors` option are persistent. Configuration loading is strict: only the
current structure size and version are accepted. Configuration writes are
considered successful only when the complete settings block is written and
the file closes successfully.

Version-1 and version-2 configuration files are therefore not interpreted
as current version-3 settings; current defaults are used until settings are
saved again.

## Building

The project uses the Raspberry Pi Pico SDK and CMake.

The current `CMakeLists.txt` targets RP2350 and provides these relevant
build options:

``` text
PICO_BOARD
VGA
HDMI
SOFTTV
I2S
I2S_CS4334
HWAY
```

Only one video backend and one audio configuration should be selected
for a release build.

The checked-in default CMake selection for 3.0.8 is MURMULATOR 2 with HDMI
enabled; `build_all.bat` still produces the full 48-configuration release
matrix described below.

Example configuration:

``` bat
cmake -S . -B build\m1-vga-pwm -G Ninja ^
  -DPICO_BOARD=murmulator ^
  -DVGA=ON ^
  -DHDMI=OFF ^
  -DSOFTTV=OFF ^
  -DI2S=OFF ^
  -DI2S_CS4334=OFF ^
  -DHWAY=OFF
cmake --build build\m1-vga-pwm
```

### Build all release configurations

`build_all.bat` builds the supported RP2350/Cortex-M33 release matrix:

-   boards: `murmulator`, `murmulator2`, `olimex-pico-pc`,
    `waveshare_rp2350_pizero`
-   video: VGA, HDMI, TV-SOFT
-   audio: PWM, I2S, I2S-CS4334, hardware AY-3-8910

This gives **48 release configurations**.

RP2040, RISC-V, `m1p2launcher`, TFT/ILI9341 and the legacy `TV` backend
are intentionally excluded from the current release matrix.

Generated binaries are placed under:

``` text
bin/Release/
```

## Firmware file names

### Board/platform prefix

-   `m1` --- MURMULATOR 1.x
-   `m2` --- MURMULATOR 2.0
-   `PC` --- Olimex RP2040-PICO-PC board family
-   `z0` --- RP2350-PiZero
-   `p2` --- Raspberry Pi Pico 2 / RP2350 generation

Examples:

``` text
m1p2-gamate-VGA-PWM-3.0.8.uf2
m2p2-gamate-HDMI-I2S-3.0.8.uf2
PCp2-gamate-VGA-AY-3-8910-3.0.8.uf2
z0p2-gamate-TV-SOFT-I2S-3.0.8.uf2
```

### Video suffix

-   `-VGA` --- VGA output
-   `-HDMI` --- HDMI output
-   `-TV-SOFT` --- software-generated composite TV/AV output

### Audio suffix

-   `-PWM` --- PWM audio
-   `-I2S` --- standard I2S audio
-   `-I2S-CS4334` --- CS4334 I2S configuration
-   `-AY-3-8910` --- hardware AY-3-8910 / TurboSound

### Extension

-   `.uf2` --- firmware image for direct installation on the RP2350/Pico
    2 target

## Installing firmware

Put the RP2350/Pico 2 board into its USB bootloader mode, connect it to
the computer, and copy the appropriate `.uf2` file to the exposed RP2350
mass-storage device.

Choose the firmware whose board, video, and audio suffixes match the
hardware configuration.

## Default hardware pin assignments

The current top-level build defines:

  Function                     GPIO
  -------------------------- ------
  SD SPI0 CS                      5
  SD SPI0 SCK                     2
  SD SPI0 MOSI                    3
  SD SPI0 MISO                    4
  PS/2 keyboard first GPIO        0
  NES CLK                        14
  NES DATA                       16
  NES LAT                        15
  VGA base GPIO                   6
  HDMI base GPIO                  6

Board-specific definitions may add or override hardware details,
including RP2350 QSPI PSRAM configuration.

## Source tree

``` text
boards/                 board definitions
drivers/audio/          audio backend
drivers/fatfs/          FAT filesystem support
drivers/graphics/       common graphics interface
drivers/hdmi/           HDMI output
drivers/nespad/         NES controller
drivers/ps2kbd/         PS/2 keyboard
drivers/sdcard/         SD-card interface
drivers/tv-software/    PAL/NTSC TV-SOFT output
drivers/vga-nextgen/    VGA output
src/                    emulator and UI
```

Some legacy backends remain in the source tree but are not part of the
current release matrix.

## Release notes

See [`release-notes.md`](release-notes.md) for the changes in the
current release.

## Notes

This project is aimed at physical RP2350/MURMULATOR hardware. Select the
firmware variant that matches the actual board pinout and installed
video/audio hardware.

The `m1` prefix is also used for **MURMULATOR Ultimate v2.x**, because
that hardware follows the MURMULATOR 1.x pinout.
