#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct __attribute__((__packed__)) {
    uint8_t version;
    bool swap_ab;
    uint8_t aspect_ratio;
    uint8_t gray_lines;
    uint8_t ghosting;
    uint8_t palette;
    uint8_t save_slot;
    uint16_t tba;
    uint32_t rgb0;
    uint32_t rgb1;
    uint32_t rgb2;
    uint32_t rgb3;
    bool instant_ignition;
    uint8_t gray_level;
    uint8_t tv_system;
    uint8_t demo_duration;
    bool color_mode;
} SETTINGS;

extern SETTINGS settings;
