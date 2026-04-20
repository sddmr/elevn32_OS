#include "framebuffer.h"
#include "font.h"
#include "string.h"
#include "serial.h"

namespace fb {

struct Framebuffer {
    uint32_t *address;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
};

static Framebuffer screen;
static uint32_t backbuffer[1280 * 800];
static uint64_t cursor_x = 0;
static uint64_t cursor_y = 0;
static uint32_t current_fg = 0x00FF00;
static uint32_t current_bg = 0x000000;

void init(uint32_t *addr, uint64_t w, uint64_t h, uint64_t pitch) {
    screen.address = addr;
    screen.width = w;
    screen.height = h;
    screen.pitch = pitch;
    cursor_x = 0;
    cursor_y = 0;
}

void swap_buffers() {
    if (screen.pitch == screen.width * 4) {
        uint32_t *dest = (uint32_t *)screen.address;
        uint32_t *src = backbuffer;
        uint64_t total_dwords = screen.width * screen.height;
        asm volatile("rep movsd" : "+D"(dest), "+S"(src) : "c"(total_dwords) : "memory");
    } else {
        for (uint32_t y = 0; y < screen.height; y++) {
            uint32_t *dest = (uint32_t *)((uint8_t *)screen.address + y * screen.pitch);
            uint32_t *src = &backbuffer[y * screen.width];
            asm volatile("rep movsd" : "+D"(dest), "+S"(src) : "c"(screen.width) : "memory");
        }
    }
}

void swap_buffers_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (x >= screen.width || y >= screen.height) return;
    if (x + w > screen.width) w = screen.width - x;
    if (y + h > screen.height) h = screen.height - y;

    for (uint32_t cy = y; cy < y + h; cy++) {
        uint32_t *dest = (uint32_t *)((uint8_t *)screen.address + cy * screen.pitch) + x;
        uint32_t *src = &backbuffer[cy * screen.width + x];
        asm volatile("rep movsd" : "+D"(dest), "+S"(src) : "c"(w) : "memory");
    }
}

void put_pixel(uint64_t x, uint64_t y, uint32_t color) {
    if (x >= screen.width || y >= screen.height) return;
    backbuffer[y * screen.width + x] = color;
}

void fill_rect(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint32_t color) {
    for (uint64_t dy = 0; dy < h; dy++) {
        for (uint64_t dx = 0; dx < w; dx++) {
            put_pixel(x + dx, y + dy, color);
        }
    }
}

void draw_image(uint64_t x, uint64_t y, uint64_t w, uint64_t h, const uint32_t* pixels) {
    for (uint64_t cy = 0; cy < h; cy++) {
        for (uint64_t cx = 0; cx < w; cx++) {
            uint32_t color = pixels[cy * w + cx];
            if (color != 0xFF00FF) { // 0xFF00FF is magic transparent key
                put_pixel(x + cx, y + cy, color);
            }
        }
    }
}

void draw_char(uint64_t x, uint64_t y, char c, uint32_t fg, uint32_t bg) {
    const uint8_t *glyph = font::get_glyph(c);
    for (int row = 0; row < font::CHAR_HEIGHT; row++) {
        for (int col = 0; col < font::CHAR_WIDTH; col++) {
            uint32_t color = (glyph[row] & (0x80 >> col)) ? fg : bg;
            put_pixel(x + col, y + row, color);
        }
    }
}

void draw_string(uint64_t x, uint64_t y, const char *str, uint32_t fg, uint32_t bg) {
    while (*str) {
        draw_char(x, y, *str, fg, bg);
        x += font::CHAR_WIDTH;
        str++;
    }
    swap_buffers();
}

void clear(uint32_t color) {
    for (uint64_t y = 0; y < screen.height; y++) {
        for (uint64_t x = 0; x < screen.width; x++) {
            put_pixel(x, y, color);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
    swap_buffers();
}

static void scroll() {
    uint32_t row_height = font::CHAR_HEIGHT;
    uint32_t *addr = backbuffer;
    uint32_t elements_to_move = (screen.height - row_height) * screen.width;
    
    for (uint32_t i = 0; i < elements_to_move; i++) {
        addr[i] = addr[i + row_height * screen.width];
    }
    
    uint32_t elements_to_clear = row_height * screen.width;
    for (uint32_t i = 0; i < elements_to_clear; i++) {
        addr[elements_to_move + i] = current_bg;
    }
}

static void newline() {
    cursor_x = 0;
    cursor_y += font::CHAR_HEIGHT;
    if (cursor_y + font::CHAR_HEIGHT > screen.height) {
        scroll();
        cursor_y -= font::CHAR_HEIGHT;
    }
}

void backspace() {
    if (cursor_x >= (uint64_t)font::CHAR_WIDTH) {
        cursor_x -= font::CHAR_WIDTH;
        draw_char(cursor_x, cursor_y, ' ', current_bg, current_bg);
    }
    swap_buffers();
}

void print(const char *str) {
    serial::print(str);
    while (*str) {
        if (*str == '\n') {
            newline();
        } else if (*str == '\b') {
            backspace();
        } else {
            draw_char(cursor_x, cursor_y, *str, current_fg, current_bg);
            cursor_x += font::CHAR_WIDTH;
            if (cursor_x + font::CHAR_WIDTH > screen.width) {
                newline();
            }
        }
        str++;
    }
    swap_buffers();
}

void println(const char *str) {
    print(str);
    newline();
    swap_buffers();
}

void set_color(uint32_t fg, uint32_t bg) {
    current_fg = fg;
    current_bg = bg;
}

}
