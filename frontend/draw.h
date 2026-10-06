#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>

#include "platform.h"

/* lr-gpsp-nspire's menu colors. */
#define COLOR_BG            RGB565(2, 8, 10)
#define COLOR_ROM_INFO      RGB565(22, 36, 26)
#define COLOR_ACTIVE_ITEM   RGB565(31, 63, 31)
#define COLOR_INACTIVE_ITEM RGB565(13, 40, 18)
#define COLOR_HELP_TEXT     RGB565(16, 40, 24)
#define COLOR_WARNING       RGB565(31, 40, 10)
#define COLOR_BLACK         RGB565(0, 0, 0)
#define COLOR_WHITE         RGB565(31, 63, 31)

#define TEXT_W 6
#define TEXT_H 10
#define TEXT_COLUMNS (SCREEN_W / TEXT_W)

void draw_clear(uint16_t color);
void draw_rect(int x, int y, int w, int h, uint16_t color);
/* Draws text with an opaque background, padded with spaces to at least
 * 'pad' characters. Stops at the right edge of the screen. */
void draw_text(const char *text, uint16_t fg, uint16_t bg, int x, int y, int pad);

#endif
