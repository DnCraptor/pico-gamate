cmake_minimum_required(VERSION 3.16)
# orange branch: Linux ARM64 port (Orange Pi Zero 2W) - DRM/KMS video,
# ALSA sound, evdev keyboards and game controllers.  The RP2350 firmware
# is built from the main branch.
project(gamate VERSION 3.1.2 LANGUAGES C CXX)

set(PROG_VERSION "3.1.2")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_C_FLAGS_RELEASE "-O2 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -g")

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBDRM REQUIRED IMPORTED_TARGET libdrm)
pkg_check_modules(ALSA REQUIRED IMPORTED_TARGET alsa)
find_package(Threads REQUIRED)

add_executable(gamate
    # Emulator
    src/main.cpp
    src/gamate/vdp.cpp
    src/m6502/m6502.c
    src/emu2149/emu2149.c

    # Text output helpers shared with the RP2350 build
    drivers/graphics/graphics.c

    # Linux host (counterparts of the RP2350 drivers)
    src/linux/hdmi_linux.c      # HDMI scanline composer (drivers/hdmi)
    src/linux/video_drm.c       # DRM/KMS output
    src/linux/audio_linux.c     # ALSA output (I2S)
    src/linux/input_linux.c     # evdev keyboards and gamepads (PS/2, NES pad)
    src/linux/kbd_linux.cpp     # key events -> HID keyboard reports
    src/linux/ff_posix.c        # FatFs API over the data directory (SD card)
    src/linux/platform_linux.c  # reboot / quit
)

# src/linux first: its pico.h, hdmi.h, nespad.h and ff.h replace the
# Pico SDK and RP2350 driver headers.
target_include_directories(gamate PRIVATE
    src/linux
    src
    drivers/graphics
)

target_compile_definitions(gamate PRIVATE
    _POSIX_C_SOURCE=200809L
    _DEFAULT_SOURCE=1
    ORANGE_GAMATE=1
    HDMI=1
    PICO_PROGRAM_NAME="Gamate emulator by xrip for Orange Pi Zero 2W"
    PICO_PROGRAM_VERSION_STRING="${PROG_VERSION}"
)

target_compile_options(gamate PRIVATE
    -Wall
    -Wextra
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
)

target_link_libraries(gamate PRIVATE PkgConfig::LIBDRM PkgConfig::ALSA Threads::Threads)
install(TARGETS gamate RUNTIME DESTINATION bin)
