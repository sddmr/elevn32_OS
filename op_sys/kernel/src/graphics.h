#pragma once
#include <stdint.h>
#include "framebuffer.h"

namespace graphics {

// Win95-inspired color palette
extern uint32_t current_desktop_color;
extern uint32_t COL_GRAY;
extern uint32_t COL_WHITE;
extern uint32_t COL_BLACK;
extern uint32_t COL_DARK;
extern uint32_t COL_DARKEST;
extern uint32_t COL_TITLEBAR;
extern uint32_t COL_TITLE_IA;

extern bool is_dark_theme;

void draw_rect(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint32_t color);
void draw_line_h(uint64_t x, uint64_t y, uint64_t w, uint32_t color);
void draw_line_v(uint64_t x, uint64_t y, uint64_t h, uint32_t color);
void draw_raised(int x, int y, int w, int h);
void draw_sunken(int x, int y, int w, int h);
void draw_cursor(uint64_t x, uint64_t y);

}
