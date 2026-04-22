#include "boot_screen.h"
#include "framebuffer.h"
#include "timer.h"
#include "string.h"
#include "graphics.h"
#include "logo.h"

namespace boot {

void play_animation() {
    fb::clear(0x000000);
    fb::swap_buffers();
    timer::sleep(300);

    int start_x = (1280 - LOGO_WIDTH) / 2;
    int start_y = (800 - LOGO_HEIGHT) / 2 - 20;

    // Draw logo line by line for a "scanning/revealing" animation
    for (int y = 0; y < LOGO_HEIGHT; y++) {
        for (int x = 0; x < LOGO_WIDTH; x++) {
            uint32_t color = LOGO_PIXELS[y * LOGO_WIDTH + x];
            if (color != 0xFF00FF) { // Transparent magic key
                fb::put_pixel(start_x + x, start_y + y, color);
            }
        }
        if (y % 4 == 0) {
            fb::swap_buffers(); 
            timer::sleep(5);
        }
    }
    fb::swap_buffers();

    timer::sleep(200);

    const char* text = "elevn32";
    int len = strlen(text);
    int char_w = 8;
    int char_h = 16;
    int text_x = (1280 - (len * char_w)) / 2;
    int text_y = start_y + LOGO_HEIGHT + 40;

    // Show "elevn32" text instantly
    fb::draw_string(text_x, text_y, text, 0xFFFFFF, 0x000000);
    fb::swap_buffers();
    timer::sleep(200);

    // Progress bar frame
    int bar_w = 200;
    int bar_h = 12;
    int bar_x = (1280 - bar_w) / 2;
    int bar_y = text_y + 30;

    graphics::draw_rect(bar_x, bar_y, bar_w, bar_h, graphics::COL_BLACK); // Black border
    fb::swap_buffers();
    
    // Fill progress bar smoothly
    for (int p = 0; p < bar_w - 2; p++) {
        graphics::draw_line_v(bar_x + 1 + p, bar_y + 1, bar_h - 2, 0x00B871); // Green accent
        if (p % 3 == 0) {
            fb::swap_buffers();
            timer::sleep(5);
        }
    }
    
    fb::swap_buffers();
    timer::sleep(500);
}

}
