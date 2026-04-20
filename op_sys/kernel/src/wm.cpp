#include "wm.h"
#include "graphics.h"
#include "mouse.h"
#include "framebuffer.h"
#include "string.h"
#include "timer.h"
#include "keyboard.h"
#include "assets.h"
#include "io.h"

namespace wm {

struct Window {
    bool active;
    int32_t x, y, w, h;
    const char *title;
};

static Window windows[5];
static int drag_win = -1;
static int32_t drag_off_x = 0;
static int32_t drag_off_y = 0;
static bool prev_left = false;

void init() {
    for (int i = 0; i < 5; i++) windows[i].active = false;

    // Info Window
    windows[0].x = 300; windows[0].y = 200; windows[0].w = 300; windows[0].h = 150;
    windows[0].title = "System Info";

    // Terminal Window
    windows[1].x = 400; windows[1].y = 250; windows[1].w = 500; windows[1].h = 300;
    windows[1].title = "Terminal";

    // Settings Window
    windows[2].x = 500; windows[2].y = 300; windows[2].w = 350; windows[2].h = 150;
    windows[2].title = "Settings";
}

static void draw_window(Window &win, int app_id) {
    if (!win.active) return;
    
    // Background
    fb::fill_rect(win.x + 1, win.y + 1, win.w - 2, win.h - 2, graphics::COLOR_WIN_BG);
    
    // Title bar
    fb::fill_rect(win.x + 1, win.y + 1, win.w - 2, 24, graphics::COLOR_TITLE_BG);
    
    // Title bar text (centered)
    int title_len = strlen(win.title);
    int title_x = win.x + (win.w - (title_len * 8)) / 2;
    fb::draw_string(title_x, win.y + 6, win.title, graphics::COLOR_TITLE_FG, graphics::COLOR_TITLE_BG);
    
    // Mac-style window control buttons (Left side)
    // Red (Close)
    fb::fill_rect(win.x + 10, win.y + 8, 10, 10, 0xFF5F56);
    // Yellow (Minimize)
    fb::fill_rect(win.x + 26, win.y + 8, 10, 10, 0xFFBD2E);
    // Green (Maximize)
    fb::fill_rect(win.x + 42, win.y + 8, 10, 10, 0x27C93F);

    // Inner content area (Black background for white text)
    fb::fill_rect(win.x + 4, win.y + 28, win.w - 8, win.h - 32, graphics::COLOR_WIN_LIGHT);
    
    // App Logic
    if (app_id == 0) { // Info
        fb::draw_string(win.x + 20, win.y + 45, "elevn32 OS v0.4", graphics::COLOR_WIN_TEXT, graphics::COLOR_WIN_LIGHT);
        fb::draw_string(win.x + 20, win.y + 65, "Architecture: x86_64", graphics::COLOR_WIN_TEXT, graphics::COLOR_WIN_LIGHT);
        fb::draw_string(win.x + 20, win.y + 85, "Developer: sdmr", graphics::COLOR_WIN_TEXT, graphics::COLOR_WIN_LIGHT);
    } else if (app_id == 1) { // Shell
        extern char shell_buffer[];
        fb::draw_string(win.x + 10, win.y + 35, "root@elevn32:~# ", 0x00FF88, graphics::COLOR_WIN_LIGHT);
        fb::draw_string(win.x + 10 + 16*8, win.y + 35, shell_buffer, 0xFFFFFF, graphics::COLOR_WIN_LIGHT);
    } else if (app_id == 2) { // Settings
        fb::draw_string(win.x + 20, win.y + 40, "Desktop Background Color:", graphics::COLOR_WIN_TEXT, graphics::COLOR_WIN_LIGHT);
        // Draw 3 color boxes for user to click
        graphics::draw_rect(win.x + 19, win.y + 59, 32, 32, graphics::COLOR_WIN_TEXT);
        fb::fill_rect(win.x + 20, win.y + 60, 30, 30, 0x28282D); // Grayish black
        
        graphics::draw_rect(win.x + 59, win.y + 59, 32, 32, graphics::COLOR_WIN_TEXT);
        fb::fill_rect(win.x + 60, win.y + 60, 30, 30, 0x00B871); // Mint Green
        
        graphics::draw_rect(win.x + 99, win.y + 59, 32, 32, graphics::COLOR_WIN_TEXT);
        fb::fill_rect(win.x + 100, win.y + 60, 30, 30, 0x007AFF); // Blue
    }
}

char shell_buffer[128] = {0};
int shell_len = 0;

void run() {
    uint64_t last_tick = 0;
    int32_t prev_mx = -1;
    int32_t prev_my = -1;

    while (true) {
        uint64_t current_tick = timer::get_ticks();
        if (current_tick - last_tick < 16) { // Throttle to ~60 FPS
            asm volatile("hlt");
            continue;
        }
        last_tick = current_tick;

        int32_t mx = mouse::get_x();
        int32_t my = mouse::get_y();
        bool m_left = mouse::is_left_clicked();

        // Keyboard Input for Shell
        while (keyboard::has_input()) {
            char c = keyboard::get_char();
            if (windows[1].active) { // Shell window active
                if (c == '\b' && shell_len > 0) {
                    shell_buffer[--shell_len] = 0;
                } else if (c == '\n') {
                    shell_buffer[0] = 0; // Clear on enter
                    shell_len = 0;
                } else if (c >= 32 && c <= 126 && shell_len < 127) {
                    shell_buffer[shell_len++] = c;
                    shell_buffer[shell_len] = 0;
                }
            }
        }

        // Desktop
        fb::fill_rect(0, 0, 1280, 800, graphics::current_desktop_color);

        // Logic
        if (m_left && !prev_left) {
            // Check Dock Icons
            int dock_w = 400;
            int dock_h = 64;
            int dock_x = (1280 - dock_w) / 2;
            int dock_y = 800 - dock_h - 10;
            int icon_y = dock_y + 8;
            
            if (mx >= dock_x + 20 && mx <= dock_x + 68 && my >= icon_y && my <= icon_y + 48) {
                windows[2].active = true; // Settings
            }
            if (mx >= dock_x + 88 && mx <= dock_x + 136 && my >= icon_y && my <= icon_y + 48) {
                windows[1].active = true; // Shell
            }
            if (mx >= dock_x + 156 && mx <= dock_x + 204 && my >= icon_y && my <= icon_y + 48) {
                windows[0].active = true; // Info
            }

            // Check title bars for drag and buttons
            for (int i = 4; i >= 0; i--) {
                if (windows[i].active) {
                    Window &win = windows[i];
                    // Close button
                    if (mx >= win.x + 10 && mx <= win.x + 20 && my >= win.y + 8 && my <= win.y + 18) {
                        win.active = false;
                        break;
                    }
                    
                    // Settings colors logic
                    if (i == 2) {
                        if (mx >= win.x + 20 && mx <= win.x + 50 && my >= win.y + 60 && my <= win.y + 90)
                            graphics::current_desktop_color = 0x28282D;
                        if (mx >= win.x + 60 && mx <= win.x + 90 && my >= win.y + 60 && my <= win.y + 90)
                            graphics::current_desktop_color = 0x00B871;
                        if (mx >= win.x + 100 && mx <= win.x + 130 && my >= win.y + 60 && my <= win.y + 90)
                            graphics::current_desktop_color = 0x007AFF;
                    }

                    // Drag
                    if (mx >= win.x + 3 && mx <= win.x + win.w - 3 &&
                        my >= win.y + 3 && my <= win.y + 23) {
                        drag_win = i;
                        drag_off_x = mx - win.x;
                        drag_off_y = my - win.y;
                        break;
                    }
                }
            }
        }

        if (!m_left) {
            drag_win = -1;
        }

        if (drag_win != -1) {
            windows[drag_win].x = mx - drag_off_x;
            windows[drag_win].y = my - drag_off_y;
        }

        prev_left = m_left;

        // Draw windows
        for (int i = 0; i < 5; i++) {
            draw_window(windows[i], i);
        }

        // Top Menubar
        fb::fill_rect(0, 0, 1280, 26, graphics::COLOR_WIN_BG);
        fb::draw_image(8, 4, assets::LOGO_SMALL_W, assets::LOGO_SMALL_H, assets::LOGO_SMALL);
        fb::draw_string(32, 5, "elevn32 OS", graphics::COLOR_TITLE_FG, graphics::COLOR_WIN_BG);
        fb::draw_string(130, 5, "File", graphics::COLOR_TITLE_FG, graphics::COLOR_WIN_BG);
        fb::draw_string(180, 5, "Edit", graphics::COLOR_TITLE_FG, graphics::COLOR_WIN_BG);
        fb::draw_string(230, 5, "View", graphics::COLOR_TITLE_FG, graphics::COLOR_WIN_BG);
        fb::draw_string(1220, 5, "00:00", graphics::COLOR_TITLE_FG, graphics::COLOR_WIN_BG);
        graphics::draw_line_h(0, 26, 1280, graphics::COLOR_WIN_DARK); // Separator line

        // Bottom Dock
        int dock_w = 400;
        int dock_h = 64;
        int dock_x = (1280 - dock_w) / 2;
        int dock_y = 800 - dock_h - 10;
        
        // Dock background (black) with white border
        fb::fill_rect(dock_x, dock_y, dock_w, dock_h, graphics::COLOR_WIN_DARK);
        graphics::draw_rect(dock_x, dock_y, dock_w, dock_h, graphics::COLOR_WIN_TEXT);
        
        // Draw Dock Icons
        int icon_y = dock_y + 8;
        fb::draw_image(dock_x + 20, icon_y, assets::ICON_SIZE, assets::ICON_SIZE, assets::ICON_SETTINGS);
        fb::draw_image(dock_x + 88, icon_y, assets::ICON_SIZE, assets::ICON_SIZE, assets::ICON_SHELL);
        fb::draw_image(dock_x + 156, icon_y, assets::ICON_SIZE, assets::ICON_SIZE, assets::ICON_INFO);

        // Draw cursor
        graphics::draw_cursor(mx, my);

        // Always do a full swap at 60 FPS. Completely eliminates flickering and tearing!
        fb::swap_buffers();

        prev_mx = mx;
        prev_my = my;
    }
}

}
