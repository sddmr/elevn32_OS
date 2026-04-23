#include "graphics.h"
#include "framebuffer.h"

namespace graphics {

uint32_t current_desktop_color = 0x2A1B38; // Main background purple
uint32_t COL_GRAY      = 0xC0C0C0;
uint32_t COL_WHITE     = 0xFFFFFF;
uint32_t COL_BLACK     = 0x000000;
uint32_t COL_DARK      = 0x808080;
uint32_t COL_DARKEST   = 0x404040;
uint32_t COL_TITLEBAR  = 0x2A1B38; // Active window titlebar purple
uint32_t COL_TITLE_IA  = 0x808080;

bool is_dark_theme = false;

void draw_line_h(uint64_t x, uint64_t y, uint64_t w, uint32_t color) {
    for (uint64_t i = 0; i < w; i++) fb::put_pixel(x + i, y, color);
}

void draw_line_v(uint64_t x, uint64_t y, uint64_t h, uint32_t color) {
    for (uint64_t i = 0; i < h; i++) fb::put_pixel(x, y + i, color);
}

void draw_rect(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint32_t color) {
    draw_line_h(x, y, w, color);
    draw_line_h(x, y + h - 1, w, color);
    draw_line_v(x, y, h, color);
    draw_line_v(x + w - 1, y, h, color);
}

void draw_raised(int x, int y, int w, int h) {
    draw_line_h(x, y, w, COL_WHITE);
    draw_line_v(x, y, h, COL_WHITE);
    draw_line_h(x + 1, y + h - 2, w - 2, COL_DARK);
    draw_line_v(x + w - 2, y + 1, h - 2, COL_DARK);
    draw_line_h(x, y + h - 1, w, COL_DARKEST);
    draw_line_v(x + w - 1, y, h, COL_DARKEST);
}

void draw_sunken(int x, int y, int w, int h) {
    draw_line_h(x, y, w, COL_DARK);
    draw_line_v(x, y, h, COL_DARK);
    draw_line_h(x + 1, y + h - 2, w - 2, COL_WHITE);
    draw_line_v(x + w - 2, y + 1, h - 2, COL_WHITE);
    draw_line_h(x, y + h - 1, w, COL_WHITE);
    draw_line_v(x + w - 1, y, h, COL_WHITE);
}

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
            if (pixel == '1') fb::put_pixel(x + c, y + r, 0xFFFFFF);
            else if (pixel == '2') fb::put_pixel(x + c, y + r, 0x000000);
        }
    }
}

}
