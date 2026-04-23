#include "boot_screen.h"
#include "framebuffer.h"
#include "timer.h"
#include "string.h"
#include "graphics.h"

namespace boot {

void play_animation() {
    fb::clear(0x000000);
    fb::swap_buffers();
    timer::sleep(300);

    const char* text = "elevn32 OS";
    int len = strlen(text);
    int char_w = 8;
    int text_x = (1280 - (len * char_w)) / 2;
    int text_y = 800 / 2 - 40;

    fb::draw_string(text_x, text_y, text, 0xFFFFFF, 0x000000);
    fb::swap_buffers();
    timer::sleep(200);

    // Progress bar frame
    int bar_w = 200;
    int bar_h = 12;
    int bar_x = (1280 - bar_w) / 2;
    int bar_y = text_y + 30;

    graphics::draw_rect(bar_x, bar_y, bar_w, bar_h, graphics::COL_WHITE); // White border
    fb::swap_buffers();
    
    uint32_t bar_color = graphics::current_desktop_color; // Use the dynamic theme color

    // Bouncing block animation
    int block_w = 40;
    for (int loop = 0; loop < 2; loop++) {
        // Forward
        for (int p = 0; p <= bar_w - 2 - block_w; p += 6) {
            fb::fill_rect(bar_x + 1, bar_y + 1, bar_w - 2, bar_h - 2, 0x000000);
            fb::fill_rect(bar_x + 1 + p, bar_y + 1, block_w, bar_h - 2, bar_color);
            fb::swap_buffers();
            timer::sleep(8);
        }
        // Backward
        for (int p = bar_w - 2 - block_w; p >= 0; p -= 6) {
            fb::fill_rect(bar_x + 1, bar_y + 1, bar_w - 2, bar_h - 2, 0x000000);
            fb::fill_rect(bar_x + 1 + p, bar_y + 1, block_w, bar_h - 2, bar_color);
            fb::swap_buffers();
            timer::sleep(8);
        }
    }

    // Fill entirely at the end
    for (int p = 0; p < bar_w - 2; p++) {
        graphics::draw_line_v(bar_x + 1 + p, bar_y + 1, bar_h - 2, bar_color);
        if (p % 4 == 0) {
            fb::swap_buffers();
            timer::sleep(5);
        }
    }
    
    fb::swap_buffers();
    timer::sleep(400);
}

}
