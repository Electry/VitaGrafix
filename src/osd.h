#ifndef _OSD_H_
#define _OSD_H_

typedef union {
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    } rgba;
    uint32_t uint32;
} rgba_t;


void osd_update_fb(const SceDisplayFrameBuf *fb);
void osd_set_back_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void osd_set_text_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void osd_set_text_scale(uint8_t scale);
void osd_set_text_scale_fl(float scale);

uint32_t osd_get_text_width(const char *str);

void osd_clear_screen();
void osd_draw_logo(int x, int y);
void osd_draw_rectangle(int x, int y, int width, int height);
void osd_draw_rectangle_fast(int x, int y, int width, int height);

void osd_draw_char(const char character, int x, int y);
void osd_draw_string(int x, int y, const char *str);
void osd_draw_stringf(int x, int y, const char *format, ...);

#endif
