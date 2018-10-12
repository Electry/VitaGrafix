#ifndef _DISPLAY_H_
#define _DISPLAY_H_

void osdUpdateFrameBuf(const SceDisplayFrameBuf *param);
void osdSetBgColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void osdSetTextColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void osdSetTextScale(uint8_t scale);

uint32_t osdGetTextWidth(const char *str);

void osdClearScreen();

void osdDrawCharacter(int character, int x, int y);
void osdDrawString(int x, int y, const char *str);
void osdDrawStringF(int x, int y, const char *format, ...);

#endif
