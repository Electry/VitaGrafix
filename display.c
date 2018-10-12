#include <vitasdk.h>
#include "font.h"

int vsnprintf(char *s, size_t n, const char *format, va_list arg);

#define MAX_STRING_LENGTH 512

static SceDisplayFrameBuf frameBuf;
static uint8_t fontScale = 1;
static uint32_t colorFg = 0xFFFFFFFF;
static uint32_t colorBg = 0xFF000000;

uint32_t osdBlendColor(uint32_t fg, uint32_t bg) {
    unsigned int alpha = ((fg >> 24) & 0xFF) + 1;
    unsigned int inv_alpha = 256 - ((fg >> 24) & 0xFF);
    uint32_t result = 0;
    result |= ((alpha * (fg & 0xFF) + inv_alpha * (bg & 0xFF)) >> 8) & 0xFF;
    result |= (((alpha * ((fg >> 8) & 0xFF) + inv_alpha * ((bg >> 8) & 0xFF)) >> 8) & 0xFF) << 8;
    result |= (((alpha * ((fg >> 16) & 0xFF) + inv_alpha * ((bg >> 16) & 0xFF)) >> 8) & 0xFF) << 16;
    result |= 0xFF << 24;
    return result;
}

void osdUpdateFrameBuf(const SceDisplayFrameBuf *pParam) {
    memcpy(&frameBuf, pParam, sizeof(SceDisplayFrameBuf));
}

void osdSetTextColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    colorFg = (a << 24) + (b << 16) + (g << 8) + r;
}
void osdSetBgColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    colorBg = (a << 24) + (b << 16) + (g << 8) + r;
}

void osdSetTextScale(uint8_t scale) {
    fontScale = scale;
}

uint32_t osdGetTextWidth(const char *str) {
    return strlen(str) * FONT_WIDTH * fontScale;
}

void osdClearScreen() {
    if (!((colorBg >> 24) & 0xFF)) // alpha == 0
        return;

    if (((colorBg >> 24) & 0xFF) == 0xFF) { // alpha == 255
        memset(frameBuf.base, colorBg, sizeof(uint32_t) * (frameBuf.pitch * frameBuf.height));
    } else {
        for (int y = 0; y < frameBuf.height; y++) {
            for (int x = 0; x < frameBuf.width; x++) {
                uint32_t *frameBufPtr = (uint32_t *)frameBuf.base + y * frameBuf.pitch + x;
                *frameBufPtr = osdBlendColor(colorBg, *frameBufPtr);
            }
        }
    }
}

void osdDrawCharacter(int character, int x, int y) {
    for (int yy = 0; yy < FONT_HEIGHT * fontScale; yy++) {
        int yy_font = yy / fontScale;
        uint32_t displacement = x + (y + yy) * frameBuf.pitch;
        uint32_t *screenPos = (uint32_t *)frameBuf.base + displacement;

        if (displacement >= frameBuf.pitch * frameBuf.height)
            return; // out of bounds

        for (int xx = 0; xx < FONT_WIDTH * fontScale; xx++) {
            if (x + xx >= frameBuf.width)
                return; // oob

            // Get px 0/1 from font.h
            int xx_font = xx / fontScale;
            uint32_t charPos = character * (FONT_HEIGHT * ((FONT_WIDTH / 8) + 1));
            uint32_t charPosH = charPos + (yy_font * ((FONT_WIDTH / 8) + 1));
            uint8_t charByte = font[charPosH + (xx_font / 8)];
            uint32_t clr = ((charByte >> (7 - (xx_font % 8))) & 1) ? colorFg : colorBg;

            if ((clr >> 24) & 0xFF) { // alpha != 0
                if (((clr >> 24) & 0xFF) != 0xFF) { // alpha < 0xFF
                    *(screenPos + xx) = osdBlendColor(clr, *(screenPos + xx)); // blend FG/BG color
                } else {
                    *(screenPos + xx) = clr;
                }
            }
        }
    }
}

void osdDrawString(int x, int y, const char *str) {
    for (size_t i = 0; i < strlen(str); i++)
        osdDrawCharacter(str[i], x + i * FONT_WIDTH * fontScale, y);
}

void osdDrawStringF(int x, int y, const char *format, ...) {
    char buffer[MAX_STRING_LENGTH] = "";
    va_list va;

    va_start(va, format);
    vsnprintf(buffer, MAX_STRING_LENGTH, format, va);
    va_end(va);

    osdDrawString(x, y, buffer);
}
