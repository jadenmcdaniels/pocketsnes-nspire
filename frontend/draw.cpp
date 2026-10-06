#include <stdint.h>
#include <string.h>

#include "draw.h"
#include "font.h"

void draw_clear(uint16_t color)
{
    uint16_t *p = platform_screen();
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++)
        p[i] = color;
}

void draw_rect(int x, int y, int w, int h, uint16_t color)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w <= 0 || h <= 0)
        return;

    uint16_t *row = platform_screen() + y * SCREEN_W + x;
    for (int j = 0; j < h; j++, row += SCREEN_W)
        for (int i = 0; i < w; i++)
            row[i] = color;
}

void draw_text(const char *text, uint16_t fg, uint16_t bg, int x, int y, int pad)
{
    uint16_t *screen = platform_screen();
    int len = (int) strlen(text);
    int count = len > pad ? len : pad;

    if (y < 0 || y + FONT_HEIGHT > SCREEN_H)
        return;

    for (int i = 0; i < count; i++, x += FONT_WIDTH)
    {
        if (x < 0)
            continue;
        if (x + FONT_WIDTH > SCREEN_W)
            break;

        unsigned char c = i < len ? (unsigned char) text[i] : ' ';
        const uint16_t *glyph = _font_bits + _font_offset[c];
        uint16_t *out = screen + y * SCREEN_W + x;
        for (int row = 0; row < FONT_HEIGHT; row++, out += SCREEN_W)
            for (int col = 0; col < FONT_WIDTH; col++)
                out[col] = ((glyph[row] >> (15 - col)) & 1) ? fg : bg;
    }
}
