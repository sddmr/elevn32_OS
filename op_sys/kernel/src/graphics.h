#pragma once
#include <stdint.h>
#include "framebuffer.h"

namespace graphics {

// Custom elevn32 Aesthetic Colors
extern uint32_t current_desktop_color; // Mutable desktop background
constexpr uint32_t COLOR_WIN_BG    = 0xEFEFEF; // Very light gray window body
constexpr uint32_t COLOR_WIN_DARK  = 0x000000; // Black outer window border
constexpr uint32_t COLOR_WIN_LIGHT = 0x000000; // Black inner content area (for white text)
constexpr uint32_t COLOR_WIN_TEXT  = 0xFFFFFF; // White text inside window
constexpr uint32_t COLOR_TITLE_BG  = 0xEFEFEF; // Light gray title bar
constexpr uint32_t COLOR_TITLE_FG  = 0x000000; // Black title text (visible on EFEFEF)
constexpr uint32_t COLOR_ACCENT    = 0x2FDF84; // Requested Green accent

void draw_rect(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint32_t color);
void draw_line_h(uint64_t x, uint64_t y, uint64_t w, uint32_t color);
void draw_line_v(uint64_t x, uint64_t y, uint64_t h, uint32_t color);
void draw_win95_border(uint64_t x, uint64_t y, uint64_t w, uint64_t h, bool sunken = false);
void draw_cursor(uint64_t x, uint64_t y);

}
