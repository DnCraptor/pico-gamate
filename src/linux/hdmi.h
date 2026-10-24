#pragma once

/*
 * Linux counterpart of drivers/hdmi/hdmi.h.
 *
 * The RP2350 driver builds every HDMI scanline of a 640x480 frame from a
 * logical 320x240 raster (each logical pixel and line is doubled by the TMDS
 * transport).  hdmi_linux.c builds the same logical lines with the same
 * rules and doubles them into a 640x480 XRGB8888 frame, which the render
 * thread hands to the DRM/KMS output (video_drm.c).
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdbool.h>

#define TEXTMODE_COLS 53
#define TEXTMODE_ROWS 30

#define RGB888(r, g, b) ((r<<16) | (g << 8 ) | b )

/* Physical frame produced by hdmi_linux_compose(), as on the RP2350 HDMI
 * output: 640x480. */
#define HDMI_FRAME_WIDTH  640
#define HDMI_FRAME_HEIGHT 480

static inline void graphics_set_flashmode(bool flash_line, bool flash_frame) {
    (void)flash_line;
    (void)flash_frame;
}

/* Build one complete 640x480 XRGB8888 frame (tightly packed) from the
 * current graphics buffer, text buffer, palette and mode. */
void hdmi_linux_compose(uint32_t *frame);

#ifdef __cplusplus
}
#endif
