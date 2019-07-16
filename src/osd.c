#include <vitasdk.h>
#include <stdio.h>

#include "osd.h"
#include "osd_font.h"
#include "osd_logo.h"

#define OSD_MAX_STRING_LENGTH  1024

static SceDisplayFrameBuf g_framebuf;
static float g_font_scale = 1.0f;

static rgba_t g_color_text = {.rgba = {255, 255, 255, 255}};
static rgba_t g_color_bg   = {.rgba = {  0,   0,   0, 255}};

rgba_t osd_blend_color(rgba_t fg, rgba_t bg) {
    uint8_t inv_alpha = 255 - fg.rgba.a;

    rgba_t result;
    result.rgba.b = ((fg.rgba.a * fg.rgba.b + inv_alpha * bg.rgba.b) >> 8); // B
    result.rgba.g = ((fg.rgba.a * fg.rgba.g + inv_alpha * bg.rgba.g) >> 8); // G
    result.rgba.r = ((fg.rgba.a * fg.rgba.r + inv_alpha * bg.rgba.r) >> 8); // R
    result.rgba.a = 0xFF;                                                   // A
    return result;
}

void osd_update_fb(const SceDisplayFrameBuf *fb) {
    memcpy(&g_framebuf, fb, sizeof(SceDisplayFrameBuf));
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

void osd_set_text_scale(uint8_t scale) {
    g_font_scale = (float)scale;
}
void osd_set_text_scale_fl(float scale) {
    g_font_scale = scale;
}

uint32_t osd_get_text_width(const char *str) {
    return strlen(str) * FONT_WIDTH * g_font_scale;
}

void osd_draw_logo(int x, int y) {
    for (int yy = y; yy < y + LOGO_HEIGHT; yy++) {
        for (int xx = x; xx < x + LOGO_WIDTH; xx++) {
            rgba_t *pixel_rgb = (rgba_t *)g_framebuf.base + yy * g_framebuf.pitch + xx;
            rgba_t *logo_rgba = (rgba_t *)g_logo + (yy - y) * LOGO_WIDTH + (xx - x);

            if (logo_rgba->rgba.a) { // alpha != 0
                if (logo_rgba->rgba.a != 0xFF) { // alpha < 255
                    *pixel_rgb = osd_blend_color(*logo_rgba, *pixel_rgb);
                } else {
                    *pixel_rgb = *logo_rgba;
                }
            }
        }
    }
}

void osd_clear_screen() {
    if (g_color_bg.rgba.a == 255) {
        // Faster
        memset(g_framebuf.base, g_color_bg.uint32,
                sizeof(uint32_t) * (g_framebuf.pitch * g_framebuf.height));
        return;
    }

    osd_draw_rectangle(0, 0, g_framebuf.width, g_framebuf.height);
}

void osd_draw_rectangle_fast(int x, int y, int width, int height) {
    if (g_color_bg.rgba.a == 0 || g_color_bg.rgba.a == 255) {
        osd_draw_rectangle(x, y, width, height);
    }

    for (int yy = y; yy < y + height; yy += 2) {
        for (int xx = x; xx < x + width; xx += 2) {
            rgba_t *pixel_rgb = (rgba_t *)g_framebuf.base + yy * g_framebuf.pitch + xx;
            rgba_t new_color = osd_blend_color(g_color_bg, *pixel_rgb);
            *pixel_rgb = new_color;
            *(pixel_rgb + 1) = new_color;
            *(pixel_rgb + g_framebuf.pitch) = new_color;
            *(pixel_rgb + g_framebuf.pitch + 1) = new_color;
        }
    }
}

void osd_draw_rectangle(int x, int y, int width, int height) {
    if (g_color_bg.rgba.a == 0)
        return;

    if (g_color_bg.rgba.a == 255) {
        // Faster
        for (int yy = y; yy < y + height; yy++) {
            memset((uint32_t *)g_framebuf.base + (yy + y) * g_framebuf.pitch + x,
                    g_color_bg.uint32,
                    sizeof(uint32_t) * width);
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

void osd_draw_char(const char character, int x, int y) {
    for (int yy = 0; yy < FONT_HEIGHT * g_font_scale; yy++) {
        int yy_font = yy / g_font_scale;
        uint32_t displacement = x + (y + yy) * g_framebuf.pitch;
        rgba_t *screen_rgb = (rgba_t *)g_framebuf.base + displacement;

        if (displacement >= g_framebuf.pitch * g_framebuf.height)
            return; // out of bounds

        for (int xx = 0; xx < FONT_WIDTH * g_font_scale; xx++) {
            if (x + xx >= g_framebuf.width)
                return; // out of bounds

            // Get px 0/1 from osd_font.h
            int xx_font = xx / g_font_scale;
            uint32_t char_pos = character * (FONT_HEIGHT * (((FONT_WIDTH - 1) / 8) + 1));
            uint32_t char_pos_h = char_pos + (yy_font * (((FONT_WIDTH - 1) / 8) + 1));
            uint8_t char_byte = g_font[char_pos_h + (xx_font / 8)];

            rgba_t clr = ((char_byte >> (7 - (xx_font % 8))) & 1) ? g_color_text : g_color_bg;

            if (clr.rgba.a) { // alpha != 0
                if (clr.rgba.a != 0xFF) { // alpha < 255
                    *(screen_rgb + xx) = osd_blend_color(clr, *(screen_rgb + xx)); // blend FG/BG color
                } else {
                    *(screen_rgb + xx) = clr;
                }
            }
        }
    }
}

void osd_draw_string(int x, int y, const char *str) {
    size_t i_cur_line = 0;

    for (size_t i = 0; i < strlen(str); i++) {
        if (str[i] == '\n') {
            i_cur_line = 0;
            y += FONT_HEIGHT;
            continue;
        }

        osd_draw_char(str[i], x + (i_cur_line * FONT_WIDTH * g_font_scale), y);
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
