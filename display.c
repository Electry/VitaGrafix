#include <psp2/types.h>
#include <psp2/display.h>
#include <libk/stdio.h>
#include <libk/stdarg.h>
#include <libk/string.h>
#include "font.h"

#define FONT_COLOR 0x00FFFFFF

unsigned int *vram32;
int pwidth, pheight, bufferwidth;
uint32_t color = FONT_COLOR;

void updateFramebuf(const SceDisplayFrameBuf *param)
{
	pwidth = param->width;
	pheight = param->height;
	vram32 = param->base;
	bufferwidth = param->pitch;
}

void setTextColor(uint32_t clr)
{
	color = clr;
}

void drawCharacter(int character, int x, int y)
{
    for (int yy = 0; yy < 10; yy++) {
        int xDisplacement = x;
        int yDisplacement = (y + (yy << 1)) * bufferwidth;
        uint32_t *screenPos = (uint32_t *)(vram32 + xDisplacement + yDisplacement);

        uint8_t charPos = font[character * 10 + yy];
        for (int xx = 7; xx >= 2; xx--) {
			uint32_t clr = ((charPos >> xx) & 1) ? color : 0xFF000000;
			*(screenPos) = clr;
			*(screenPos + 1) = clr;
			*(screenPos + bufferwidth) = clr;
			*(screenPos + bufferwidth + 1) = clr;
			screenPos += 2;
        }
    }
}

void drawString(int x, int y, const char *str)
{
    for (size_t i = 0; i < strlen(str); i++)
        drawCharacter(str[i], x + i * 12, y);
}

void drawStringF(int x, int y, const char *format, ...)
{
	char str[512] = { 0 };
	va_list va;

	va_start(va, format);
	vsnprintf(str, 512, format, va);
	va_end(va);

	drawString(x, y, str);
}

