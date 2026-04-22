#pragma once
#include <stdint.h>

namespace fb {
    void init(uint32_t *addr, uint64_t w, uint64_t h, uint64_t pitch);
    void put_pixel(uint64_t x, uint64_t y, uint32_t color);
    void fill_rect(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint32_t color);
    void draw_char(uint64_t x, uint64_t y, char c, uint32_t fg, uint32_t bg);
    void draw_string(uint64_t x, uint64_t y, const char *str, uint32_t fg, uint32_t bg);
    void clear(uint32_t color);
    void print(const char *str);
    void println(const char *str);
    void set_color(uint32_t fg, uint32_t bg);
    void backspace();
    void swap_buffers();
    void swap_buffers_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    void draw_image(uint64_t x, uint64_t y, uint64_t w, uint64_t h, const uint32_t* pixels);
    uint32_t get_pixel_raw(uint64_t x, uint64_t y);
}
