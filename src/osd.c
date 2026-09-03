#include <vitasdk.h>
#include <stdio.h>
#include <string.h>

#include "osd.h"
#include "osd_font.h"

#define OSD_MAX_STRING_LENGTH  1024
#define OSD_RESCALE_X(x) (int)((x) * (g_framebuf.width / 960.0f))
#define OSD_RESCALE_Y(y) (int)((y) * (g_framebuf.height / 544.0f))

typedef union {
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    } rgba;
    uint32_t uint32;
} rgba_t;

static SceDisplayFrameBuf g_framebuf;
static const bitmap_font_t *g_font = &g_fonts[0];

static rgba_t g_color_text = {.rgba = {255, 255, 255, 255}};
static rgba_t g_color_bg   = {.rgba = {  0,   0,   0, 255}};

static void osd_draw_rectangle_abs(int x, int y, int width, int height);

static rgba_t osd_blend_color(rgba_t fg, rgba_t bg) {
    uint8_t inv_alpha = 255 - fg.rgba.a;

    rgba_t result;
    result.rgba.b = ((fg.rgba.a * fg.rgba.b + inv_alpha * bg.rgba.b) >> 8); // B
    result.rgba.g = ((fg.rgba.a * fg.rgba.g + inv_alpha * bg.rgba.g) >> 8); // G
    result.rgba.r = ((fg.rgba.a * fg.rgba.r + inv_alpha * bg.rgba.r) >> 8); // R
    result.rgba.a = 0xFF;                                                   // A
    return result;
}

static void osd_fill_color(rgba_t *pixels, int count) {
    if (g_color_bg.rgba.r == g_color_bg.rgba.g
            && g_color_bg.rgba.r == g_color_bg.rgba.b
            && g_color_bg.rgba.r == g_color_bg.rgba.a) {
        memset(pixels, g_color_bg.rgba.r, sizeof(rgba_t) * count);
        return;
    }

    for (int i = 0; i < count; i++) {
        pixels[i] = g_color_bg;
    }
}

void osd_update_fb(const SceDisplayFrameBuf *fb) {
    memcpy(&g_framebuf, fb, sizeof(SceDisplayFrameBuf));

    if (fb->width <= 480) {
        g_font = &g_fonts[3];
    } else if (fb->width <= 640) {
        g_font = &g_fonts[2];
    } else if (fb->width <= 720) {
        g_font = &g_fonts[1];
    } else {
        g_font = &g_fonts[0];
    }
}

void osd_set_text_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    g_color_text.rgba.r = r;
    g_color_text.rgba.g = g;
    g_color_text.rgba.b = b;
    g_color_text.rgba.a = a;
}

void osd_set_back_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    g_color_bg.rgba.r = r;
    g_color_bg.rgba.g = g;
    g_color_bg.rgba.b = b;
    g_color_bg.rgba.a = a;
}

static uint32_t osd_get_text_width_abs(const char *str) {
    return strlen(str) * g_font->width;
}

uint32_t osd_get_text_width(const char *str) {
    return osd_get_text_width_abs(str) * 960.0f / g_framebuf.width;
}

uint32_t osd_get_text_height() {
    return g_font->height * 544.0f / g_framebuf.height;
}

int osd_get_text_end_x(int x, const char *str) {
    int end_x = OSD_RESCALE_X(x) + osd_get_text_width_abs(str);
    return (end_x * 960 + g_framebuf.width - 1) / g_framebuf.width;
}

void osd_clear_screen() {
    if (g_color_bg.rgba.a == 255) {
        osd_fill_color((rgba_t *)g_framebuf.base, g_framebuf.pitch * g_framebuf.height);
        return;
    }

    osd_draw_rectangle_abs(0, 0, g_framebuf.width, g_framebuf.height);
}

void osd_draw_rectangle_fast(int x, int y, int width, int height) {
    if (g_color_bg.rgba.a == 0 || g_color_bg.rgba.a == 255) {
        osd_draw_rectangle(x, y, width, height);
        return;
    }

    x = OSD_RESCALE_X(x);
    y = OSD_RESCALE_Y(y);
    width = OSD_RESCALE_X(width);
    height = OSD_RESCALE_Y(height);

    for (int yy = y; yy < y + height; yy += 2) {
        for (int xx = x; xx < x + width; xx += 2) {
            rgba_t *pixel_rgb = (rgba_t *)g_framebuf.base + yy * g_framebuf.pitch + xx;
            rgba_t new_color = osd_blend_color(g_color_bg, *pixel_rgb);
            *pixel_rgb = new_color;
            if (xx + 1 < x + width) {
                *(pixel_rgb + 1) = new_color;
            }
            if (yy + 1 < y + height) {
                *(pixel_rgb + g_framebuf.pitch) = new_color;
                if (xx + 1 < x + width) {
                    *(pixel_rgb + g_framebuf.pitch + 1) = new_color;
                }
            }
        }
    }
}

void osd_draw_rectangle(int x, int y, int width, int height) {
    x = OSD_RESCALE_X(x);
    y = OSD_RESCALE_Y(y);
    width = OSD_RESCALE_X(width);
    height = OSD_RESCALE_Y(height);

    osd_draw_rectangle_abs(x, y, width, height);
}

void osd_draw_rounded_rectangle(int x, int y, int width, int height, int radius) {
    x = OSD_RESCALE_X(x);
    y = OSD_RESCALE_Y(y);
    width = OSD_RESCALE_X(width);
    height = OSD_RESCALE_Y(height);
    int radius_x = OSD_RESCALE_X(radius);
    int radius_y = OSD_RESCALE_Y(radius);

    if (g_color_bg.rgba.a == 0)
        return;

    if (radius_x > width / 2)
        radius_x = width / 2;
    if (radius_y > height / 2)
        radius_y = height / 2;

    for (int yy = 0; yy < height; yy++) {
        int inset = 0;

        if (yy < radius_y || yy >= height - radius_y) {
            int dy = yy < radius_y ? radius_y - yy : yy - (height - radius_y - 1);

            while (inset < radius_x) {
                int dx = radius_x - inset;
                if (dx * dx * radius_y * radius_y + dy * dy * radius_x * radius_x
                        <= radius_x * radius_x * radius_y * radius_y)
                    break;
                inset++;
            }
        }

        rgba_t *pixels = (rgba_t *)g_framebuf.base + (y + yy) * g_framebuf.pitch + x + inset;
        int count = width - inset * 2;
        if (g_color_bg.rgba.a == 255) {
            osd_fill_color(pixels, count);
        } else {
            for (int xx = 0; xx < count; xx++) {
                pixels[xx] = osd_blend_color(g_color_bg, pixels[xx]);
            }
        }
    }
}

void osd_draw_header(const char *text) {
    char line[OSD_MAX_STRING_LENGTH];
    int width = 0;
    int line_count = 0;
    int text_height = osd_get_text_height();
    const char *line_begin = text;
    const char *line_end;

    while (1) {
        line_end = strchr(line_begin, '\n');
        size_t line_length = line_end ? line_end - line_begin : strlen(line_begin);
        memcpy(line, line_begin, line_length);
        line[line_length] = '\0';

        int line_width = osd_get_text_width(line);
        if (line_width > width) {
            width = line_width;
        }

        line_count++;
        if (line_end == NULL) {
            break;
        }
        line_begin = line_end + 1;
    }

    osd_set_back_color(0, 0, 0, 255);
    osd_draw_rounded_rectangle(20, 20, width + 20, line_count * text_height + (line_count - 1) * 2 + 10, 5);
    osd_set_back_color(0, 0, 0, 0);

    int y = 25;
    line_begin = text;
    while (1) {
        line_end = strchr(line_begin, '\n');
        size_t line_length = line_end ? line_end - line_begin : strlen(line_begin);
        memcpy(line, line_begin, line_length);
        line[line_length] = '\0';

        osd_draw_string(30, y, line);
        y += text_height + 2;

        if (line_end == NULL) {
            break;
        }
        line_begin = line_end + 1;
    }
}

static void osd_draw_rectangle_abs(int x, int y, int width, int height) {
    if (g_color_bg.rgba.a == 0)
        return;

    if (g_color_bg.rgba.a == 255) {
        for (int yy = y; yy < y + height; yy++) {
            osd_fill_color((rgba_t *)g_framebuf.base + yy * g_framebuf.pitch + x, width);
        }
        return;
    }

    for (int yy = y; yy < y + height; yy++) {
        for (int xx = x; xx < x + width; xx++) {
            rgba_t *pixel_rgb = (rgba_t *)g_framebuf.base + yy * g_framebuf.pitch + xx;
            *pixel_rgb = osd_blend_color(g_color_bg, *pixel_rgb);
        }
    }
}

static void osd_draw_char_abs(char character, int x, int y) {
    if (character < g_font->first_char || character > g_font->last_char)
        character = '?'; // invalid char

    const uint8_t *glyph = &g_font->data[(character - g_font->first_char) * g_font->bytes_per_glyph];
    int height = g_font->height;
    int width = g_font->width;
    int x_start = x < 0 ? -x : 0;
    int y_start = y < 0 ? -y : 0;
    int x_end = x + width > g_framebuf.width ? g_framebuf.width - x : width;
    int y_end = y + height > g_framebuf.height ? g_framebuf.height - y : height;

    if (x_start >= x_end || y_start >= y_end)
        return;

    if (g_color_text.rgba.a == 255 && g_color_bg.rgba.a == 0) {
        for (int yy = y_start; yy < y_end; yy++) {
            rgba_t *screen_rgb = (rgba_t *)g_framebuf.base + (y + yy) * g_framebuf.pitch + x + x_start;
            const uint8_t *glyph_row = glyph + yy * g_font->bytes_per_row;

            for (int byte = x_start / 8; byte <= (x_end - 1) / 8; byte++) {
                uint8_t char_byte = glyph_row[byte];
                int bit_start = byte == x_start / 8 ? x_start % 8 : 0;
                int bit_end = byte == (x_end - 1) / 8 ? (x_end - 1) % 8 + 1 : 8;

                if (char_byte == 0)
                    continue;

                for (int bit = bit_start; bit < bit_end; bit++) {
                    if (char_byte & (0x80 >> bit))
                        screen_rgb[byte * 8 + bit - x_start] = g_color_text;
                }
            }
        }
        return;
    }

    for (int yy = y_start; yy < y_end; yy++) {
        rgba_t *screen_rgb = (rgba_t *)g_framebuf.base + (y + yy) * g_framebuf.pitch + x + x_start;

        for (int xx = x_start; xx < x_end; xx++) {
            uint8_t char_byte = glyph[yy * g_font->bytes_per_row + xx / 8];

            rgba_t clr = ((char_byte >> (7 - (xx % 8))) & 1) ? g_color_text : g_color_bg;

            if (clr.rgba.a) { // alpha != 0
                if (clr.rgba.a != 0xFF) { // alpha < 255
                    screen_rgb[xx - x_start] = osd_blend_color(clr, screen_rgb[xx - x_start]); // blend FG/BG color
                } else {
                    screen_rgb[xx - x_start] = clr;
                }
            }
        }
    }
}

void osd_draw_string(int x, int y, const char *str) {
    x = OSD_RESCALE_X(x);
    y = OSD_RESCALE_Y(y);

    size_t i_cur_line = 0;

    size_t slen = strlen(str);
    for (size_t i = 0; i < slen; i++) {
        if (str[i] == '\n') {
            i_cur_line = 0;
            y += g_font->height;
            continue;
        }

        osd_draw_char_abs(str[i], x + (i_cur_line * g_font->width), y);
        i_cur_line++;
    }
}

void osd_draw_stringf(int x, int y, const char *format, ...) {
    char buffer[OSD_MAX_STRING_LENGTH] = "";
    va_list va;

    va_start(va, format);
    vsnprintf(buffer, OSD_MAX_STRING_LENGTH, format, va);
    va_end(va);

    osd_draw_string(x, y, buffer);
}

void osd_draw_log(int x, int y, int maxy, const char *str) {
    size_t slen = strlen(str);
    if (slen <= 3)
        return;

    x = OSD_RESCALE_X(x);
    y = OSD_RESCALE_Y(y);

    size_t line_end = slen - 1;

    for (int i = slen - 2; i >= 0; i--) {
        if (i == 0 || str[i - 1] == '\n') {
            maxy -= g_font->height;
            if (maxy < y)
                break;

            for (size_t i_cur = 0; i_cur < line_end - i; i_cur++) {
                osd_draw_char_abs(str[i + i_cur], x + (i_cur * g_font->width),
                                  maxy - g_font->height);
            }

            line_end = i - 1;
        }
    }
}
