#ifndef _DISPLAY_H_
#define _DISPLAY_H_

void updateFramebuf(const SceDisplayFrameBuf *param);
void drawCharacter(int character, int x, int y);
void drawString(int x, int y, const char *str);
void drawStringF(int x, int y, const char *format, ...);
void setTextColor(uint32_t clr);

#endif
