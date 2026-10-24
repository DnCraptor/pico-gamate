/*
 * Linux counterpart of drivers/hdmi/hdmi.c: the graphics_* API and the
 * scanline composer.
 *
 * On the RP2350 the DMA IRQ builds each logical 320-sample scanline into a
 * palette-index buffer, which the PIO/TMDS path doubles to 640 pixels, and
 * every logical line is sent twice (480 physical lines).  Here the same
 * logical lines are built with the same rules (modes, bezel, gray lines,
 * Demo OSD) and expanded through the palette into a 640x480 XRGB8888 frame.
 */
#include "graphics.h"

#include <string.h>

extern volatile uint8_t gamate_gray_lines;
extern volatile uint8_t gamate_gray_level;
extern volatile bool gamate_demo_title_visible;
extern volatile uint16_t gamate_demo_title_width;
extern uint8_t gamate_demo_title_bitmap[8][316];
extern volatile bool gamate_demo_countdown_visible;
extern volatile uint8_t gamate_demo_countdown_width;
extern uint8_t gamate_demo_countdown_bitmap[8][18];
#include "../../drivers/hdmi/gamate_photo.h"

static const uint8_t textmode_palette[16] = {
    200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215
};

//активный видеорежим
static enum graphics_mode_t graphics_mode = GRAPHICSMODE_DEFAULT;

//буфер палитры 256 цветов в формате R8G8B8
static uint32_t palette[256];

#define SCREEN_WIDTH (320)
#define SCREEN_HEIGHT (240)
//графический буфер
static uint8_t* graphics_buffer = NULL;
static int graphics_buffer_width = 0;
static int graphics_buffer_height = 0;
static int graphics_buffer_shift_x = 0;
static int graphics_buffer_shift_y = 0;

static uint8_t gamate_hdmi_scale_x[GAMATE_HDMI_W];
static uint8_t gamate_hdmi_scale_y[GAMATE_HDMI_H];
/* 4:3 uses the complete logical 320x240 HDMI active raster. */
static uint8_t gamate_hdmi_4x3_scale_x[SCREEN_WIDTH];
static uint8_t gamate_hdmi_4x3_scale_y[SCREEN_HEIGHT];
static uint8_t gamate_hdmi_demo_fg = 0;
static uint32_t gamate_hdmi_demo_fg_luma = 0;
static uint8_t gamate_hdmi_gray = 128;

//текстовый буфер
uint8_t* text_buffer = NULL;

#define BASE_HDMI_CTRL_INX (240)
#define GAMATE_HDMI_4X3_GRAY_INX (216)

static inline uint8_t gamate_hdmi_palette_slot(const int i) {
    return (i < 112) ? (uint8_t)(128 + i) : (uint8_t)(244 + i - 112);
}

static void gamate_hdmi_prepare_scaling(void) {
    for (int x = 0; x < SCREEN_WIDTH; x++)
        gamate_hdmi_4x3_scale_x[x] = (uint8_t)((x * 160) / SCREEN_WIDTH);
    for (int y = 0; y < SCREEN_HEIGHT; y++)
        gamate_hdmi_4x3_scale_y[y] = (uint8_t)((y * 150) / SCREEN_HEIGHT);
    for (int x = 0; x < GAMATE_HDMI_W; x++)
        gamate_hdmi_scale_x[x] = (uint8_t)((x * 160) / GAMATE_HDMI_W);
    for (int y = 0; y < GAMATE_HDMI_H; y++)
        gamate_hdmi_scale_y[y] = (uint8_t)((y * 150) / GAMATE_HDMI_H);
}

static void gamate_hdmi_load_photo_palette(void) {
    const int gray = (int)(gamate_gray_level & 3u) * 0x55;
    uint32_t gray_error = UINT32_MAX;
    if (gray == 0) gamate_hdmi_gray = 255;
    for (int i = 0; i < GAMATE_HDMI_PALETTE_SIZE; i++) {
        const uint32_t rgb = gamate_hdmi_palette_rgb[i];
        graphics_set_palette(gamate_hdmi_palette_slot(i), rgb);
        const int dr = (int)((rgb >> 16) & 0xff) - gray;
        const int dg = (int)((rgb >> 8) & 0xff) - gray;
        const int db = (int)(rgb & 0xff) - gray;
        const uint32_t error = (uint32_t)(dr * dr + dg * dg + db * db);
        if (gray != 0 && error < gray_error) {
            gray_error = error;
            gamate_hdmi_gray = gamate_hdmi_palette_slot(i);
        }
    }
}

static void gamate_hdmi_load_text_palette(void) {
    static const uint32_t colors[16] = {
        RGB888(0x00, 0x00, 0x00), RGB888(0x00, 0x00, 0xC4),
        RGB888(0x00, 0xC4, 0x00), RGB888(0x00, 0xC4, 0xC4),
        RGB888(0xC4, 0x00, 0x00), RGB888(0xC4, 0x00, 0xC4),
        RGB888(0xC4, 0x7E, 0x00), RGB888(0xC4, 0xC4, 0xC4),
        RGB888(0x4E, 0x4E, 0x4E), RGB888(0x4E, 0x4E, 0xDC),
        RGB888(0x4E, 0xDC, 0x4E), RGB888(0x4E, 0xF3, 0xF3),
        RGB888(0xDC, 0x4E, 0x4E), RGB888(0xF3, 0x4E, 0xF3),
        RGB888(0xF3, 0xF3, 0x4E), RGB888(0xFF, 0xFF, 0xFF),
    };
    for (int i = 0; i < 16; i++) graphics_set_palette(200 + i, colors[i]);
}

/* One logical 320-sample scanline (y = 0..239), as dma_handler_HDMI()
 * builds it into the active part of its DMA line buffer. */
static void compose_line(const int y, uint8_t* output_buffer) {
    uint8_t* const line_start = output_buffer;

    if (!graphics_buffer) {
        memset(output_buffer, 255, SCREEN_WIDTH);
        return;
    }

    uint8_t* input_buffer;
    switch (graphics_mode) {
        case GRAPHICSMODE_DEFAULT:
        case VGA_320x240x256: {
            //заполняем пространство сверху и снизу графического буфера
            if (y <= graphics_buffer_shift_y || y >= (graphics_buffer_shift_y + graphics_buffer_height)) {
                memset(output_buffer, 255, SCREEN_WIDTH);
                break;
            }

            uint8_t* activ_buf_end = output_buffer + SCREEN_WIDTH;
            //рисуем пространство слева от буфера
            if (graphics_buffer_shift_x > 0) {
                memset(output_buffer, 255, graphics_buffer_shift_x);
                output_buffer += graphics_buffer_shift_x;
            }

            //рисуем сам видеобуфер+пространство справа
            input_buffer = &graphics_buffer[(y - graphics_buffer_shift_y) * graphics_buffer_width];

            const uint8_t* input_buffer_end = input_buffer + graphics_buffer_width;

            if (graphics_buffer_shift_x < 0) input_buffer -= graphics_buffer_shift_x;

            while (activ_buf_end > output_buffer) {
                if (input_buffer < input_buffer_end) {
                    uint8_t i_color = *input_buffer++;
                    i_color = ((i_color & 0xf0) == 0xf0) ? 255 : i_color;
                    *output_buffer++ = i_color;
                } else
                    *output_buffer++ = 255;
            }

            break;
        }
        case GRAPHICSMODE_3X3: {
            /* HDMI 4:3: each source pixel already occupies two logical
             * samples; the HDMI transport doubles each sample again.
             * Gray mode uses those two logical samples independently. */
            input_buffer = &graphics_buffer[
                gamate_hdmi_4x3_scale_y[y] * graphics_buffer_width];
            if (gamate_gray_lines) {
                for (int x = 0; x < 160; x++) {
                    output_buffer[x * 2] = input_buffer[x];
                    output_buffer[x * 2 + 1] = GAMATE_HDMI_4X3_GRAY_INX;
                }
            } else {
                for (int x = 0; x < SCREEN_WIDTH; x++)
                    output_buffer[x] = input_buffer[gamate_hdmi_4x3_scale_x[x]];
            }
            break;
        }
        case GRAPHICSMODE_ASPECT: {
            if (y < GAMATE_HDMI_Y || y >= GAMATE_HDMI_Y + GAMATE_HDMI_H) {
                const int row = (y < GAMATE_HDMI_Y)
                    ? y
                    : GAMATE_HDMI_Y + y - (GAMATE_HDMI_Y + GAMATE_HDMI_H);
                memcpy(output_buffer, gamate_hdmi_outside + row * SCREEN_WIDTH,
                       SCREEN_WIDTH);
                break;
            }

            const int bezel_row = y - GAMATE_HDMI_Y;
            const int right_width = SCREEN_WIDTH - GAMATE_HDMI_X - GAMATE_HDMI_W;
            const int sides_stride = GAMATE_HDMI_X + right_width;
            const uint8_t* sides = gamate_hdmi_sides + bezel_row * sides_stride;

            memcpy(output_buffer, sides, GAMATE_HDMI_X);
            input_buffer = &graphics_buffer[
                gamate_hdmi_scale_y[bezel_row] * graphics_buffer_width];
            for (int x = 0; x < GAMATE_HDMI_W; x++)
                output_buffer[GAMATE_HDMI_X + x] = input_buffer[gamate_hdmi_scale_x[x]];
            memcpy(output_buffer + GAMATE_HDMI_X + GAMATE_HDMI_W,
                   sides + GAMATE_HDMI_X, right_width);
            break;
        }
        case TEXTMODE_DEFAULT:
        case TEXTMODE_53x30: {
            *output_buffer++ = 255;
            for (int x = 0; x < TEXTMODE_COLS; x++) {
                const uint16_t offset = (y / 8) * (TEXTMODE_COLS * 2) + x * 2;
                const uint8_t c = text_buffer[offset];
                const uint8_t colorIndex = text_buffer[offset + 1];
                if (c == 0 && colorIndex >= 0xF0 && colorIndex <= 0xF3) {
                    const uint8_t preview = (uint8_t)(31 + ((colorIndex & 3) << 5));
                    for (int bit = 6; bit--;) *output_buffer++ = preview;
                } else {
                    uint8_t glyph_row = font_6x8[c * 8 + y % 8];
                    for (int bit = 6; bit--;) {
                        *output_buffer++ = glyph_row & 1
                                               ? textmode_palette[colorIndex & 0xf] //цвет шрифта
                                               : textmode_palette[colorIndex >> 4]; //цвет фона
                        glyph_row >>= 1;
                    }
                }
            }
            *output_buffer = 255;
            break;
        }
        default:
            memset(output_buffer, 255, SCREEN_WIDTH);
            break;
    }

    /* Demo title is a screen overlay, independent of gameplay scaling
     * and of the bezel. */
    if ((gamate_demo_title_visible || gamate_demo_countdown_visible) &&
        graphics_mode != TEXTMODE_DEFAULT && graphics_mode != TEXTMODE_53x30 &&
        y >= 228 && y < 240) {
        uint8_t* title_dst = line_start;
        memset(title_dst, 255, SCREEN_WIDTH);
        if (y >= 230 && y < 238) {
            const int title_w = gamate_demo_title_width;
            const int title_x = (SCREEN_WIDTH - title_w) / 2;
            if (gamate_demo_title_visible && title_w > 0) {
                const uint8_t* bits = gamate_demo_title_bitmap[y - 230];
                for (int x = 0; x < title_w; ++x)
                    if (bits[x]) title_dst[title_x + x] = gamate_hdmi_demo_fg;
            }
            if (gamate_demo_countdown_visible && gamate_demo_countdown_width) {
                const int countdown_x = SCREEN_WIDTH - gamate_demo_countdown_width - 4;
                const uint8_t* bits = gamate_demo_countdown_bitmap[y - 230];
                for (int x = 0; x < gamate_demo_countdown_width; ++x)
                    if (bits[x]) title_dst[countdown_x + x] = gamate_hdmi_demo_fg;
            }
        }
    }
}

/* Logical line -> physical 640-pixel row (the TMDS path doubles samples). */
static void expand_line(uint32_t* dst, const uint8_t* line) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        const uint32_t rgb = palette[line[x]];
        dst[0] = rgb;
        dst[1] = rgb;
        dst += 2;
    }
}

void hdmi_linux_compose(uint32_t* frame) {
    uint8_t line[SCREEN_WIDTH];
    /* Gray-line mode: the second physical line of each pair shows the gray
     * colour over the live LCD span, the bezel stays as it is. */
    const bool gray_active =
        graphics_mode == GRAPHICSMODE_ASPECT && gamate_gray_lines != 0;

    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        uint32_t* row0 = frame + (size_t)(2 * y) * HDMI_FRAME_WIDTH;
        uint32_t* row1 = row0 + HDMI_FRAME_WIDTH;
        compose_line(y, line);
        expand_line(row0, line);
        if (gray_active && y >= GAMATE_HDMI_Y && y < GAMATE_HDMI_Y + GAMATE_HDMI_H) {
            memset(line + GAMATE_HDMI_X, gamate_hdmi_gray, GAMATE_HDMI_W);
            expand_line(row1, line);
        } else {
            memcpy(row1, row0, HDMI_FRAME_WIDTH * sizeof(uint32_t));
        }
    }
}

void adjust_clk(void) {
}

//выбор видеорежима
void graphics_set_mode(enum graphics_mode_t mode) {
    graphics_mode = mode;
    if (mode == GRAPHICSMODE_ASPECT) gamate_hdmi_load_photo_palette();
    else if (mode == GRAPHICSMODE_3X3) {
        const uint8_t gray = (uint8_t)((gamate_gray_level & 3u) * 0x55u);
        graphics_set_palette(GAMATE_HDMI_4X3_GRAY_INX, RGB888(gray, gray, gray));
    }
    else if (mode == TEXTMODE_DEFAULT || mode == TEXTMODE_53x30) gamate_hdmi_load_text_palette();
    clrScr(0);
}

void graphics_set_palette(uint8_t i, uint32_t color888) {
    palette[i] = color888 & 0x00ffffff;
    /* Demo title must stay readable in both HDMI game modes.  The photo
     * palette occupies 128..239 and 244..254, so never use one of those
     * entries for title text.  Instead follow the brightest currently
     * programmed Gamate colour from the stable 0..127 game palette. */
    if (i < 128) {
        const uint32_t r = (color888 >> 16) & 0xff;
        const uint32_t g = (color888 >> 8) & 0xff;
        const uint32_t bl = color888 & 0xff;
        const uint32_t luma = r * 299u + g * 587u + bl * 114u;
        if (i == 0 || luma > gamate_hdmi_demo_fg_luma) {
            gamate_hdmi_demo_fg_luma = luma;
            gamate_hdmi_demo_fg = i;
        }
    }
}

void graphics_set_buffer(uint8_t* buffer, uint16_t width, uint16_t height) {
    graphics_buffer = buffer;
    graphics_buffer_width = width;
    graphics_buffer_height = height;
}

void graphics_init(void) {
    gamate_hdmi_load_text_palette();
    gamate_hdmi_prepare_scaling();
}

void graphics_set_bgcolor(uint32_t color888) //определяем зарезервированный цвет в палитре
{
    graphics_set_palette(255, color888);
}

void graphics_set_offset(int x, int y) {
    graphics_buffer_shift_x = x;
    graphics_buffer_shift_y = y;
}

void graphics_set_textbuffer(uint8_t* buffer) {
    text_buffer = buffer;
}

void clrScr(const uint8_t color) {
    if (text_buffer)
        memset(text_buffer, color, TEXTMODE_COLS * TEXTMODE_ROWS * 2);
}
