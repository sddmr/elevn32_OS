#include "graphics.h"
#include "framebuffer.h"

namespace graphics {

uint32_t current_desktop_color = 0x28282D;

void draw_line_h(uint64_t x, uint64_t y, uint64_t w, uint32_t color) {
    for (uint64_t i = 0; i < w; i++) {
        fb::put_pixel(x + i, y, color);
    }
}

void draw_line_v(uint64_t x, uint64_t y, uint64_t h, uint32_t color) {
    for (uint64_t i = 0; i < h; i++) {
        fb::put_pixel(x, y + i, color);
    }
}

void draw_rect(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint32_t color) {
    draw_line_h(x, y, w, color);
    draw_line_h(x, y + h - 1, w, color);
    draw_line_v(x, y, h, color);
    draw_line_v(x + w - 1, y, h, color);
}

void draw_win95_border(uint64_t x, uint64_t y, uint64_t w, uint64_t h, bool sunken) {
    uint32_t top_left = sunken ? COLOR_WIN_DARK : COLOR_WIN_LIGHT;
    uint32_t bottom_right = sunken ? COLOR_WIN_LIGHT : COLOR_WIN_DARK;
    uint32_t outer_tl = sunken ? 0x000000 : COLOR_WIN_LIGHT;
    uint32_t outer_br = sunken ? COLOR_WIN_LIGHT : 0x000000;

    // Inner highlight/shadow
    draw_line_h(x + 1, y + 1, w - 2, top_left);
    draw_line_v(x + 1, y + 1, h - 2, top_left);
    draw_line_h(x + 1, y + h - 2, w - 2, bottom_right);
    draw_line_v(x + w - 2, y + 1, h - 2, bottom_right);

    // Outer highlight/shadow
    draw_line_h(x, y, w, outer_tl);
    draw_line_v(x, y, h, outer_tl);
    draw_line_h(x, y + h - 1, w, outer_br);
    draw_line_v(x + w - 1, y, h, outer_br);
}

// A simple classic arrow cursor bitmap
static const char *cursor_bitmap[] = {
    "100000000000",
    "110000000000",
    "121000000000",
    "122100000000",
    "122210000000",
    "122221000000",
    "122222100000",
    "122222210000",
    "122222221000",
    "122222222100",
    "122222222210",
    "122222211111",
    "122122210000",
    "121012221000",
    "110012221000",
    "100001222100",
    "000001222100",
    "000000111000"
};

void draw_cursor(uint64_t x, uint64_t y) {
    for (int r = 0; r < 18; r++) {
        for (int c = 0; c < 12; c++) {
            char pixel = cursor_bitmap[r][c];
            if (pixel == '1') {
                fb::put_pixel(x + c, y + r, 0xFFFFFF); // White border
            } else if (pixel == '2') {
                fb::put_pixel(x + c, y + r, 0x000000); // Black fill
            }
        }
    }
}

}
