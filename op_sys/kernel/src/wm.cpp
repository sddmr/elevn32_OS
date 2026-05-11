#include "wm.h"
#include "framebuffer.h"
#include "fs.h"
#include "graphics.h"
#include "heap.h"
#include "io.h"
#include "keyboard.h"
#include "mouse.h"
#include "shell.h"
#include "string.h"
#include "timer.h"

namespace wm {
namespace g = graphics;

enum SysState { STATE_LOGIN, STATE_DESKTOP };
static SysState current_state = STATE_LOGIN;
static char l_user[32] = "root", l_pass[32] = "";
static int l_ulen = 4, l_plen = 0, l_focus = 1;
static bool login_err = false;

struct Window {
  bool active;
  bool minimized;
  bool maximized;
  int32_t x, y, w, h;
  int32_t rx, ry, rw, rh;
  const char *title;
};
struct Rect {
  int32_t x, y, w, h;
};
constexpr int NUM_WIN = 10;
static Window windows[NUM_WIN];
static int win_order[NUM_WIN];
static uint32_t pbuf[280 * 160];
static int sh_count = 0;
static char sh_lines[25][64];
static char sh_in[64];
static int sh_ptr = 0;
static char sh_hist[8][64];
static int sh_hist_count = 0;
static int sh_hist_pos = -1;
static char notepad[1024];
static int note_len = 0;
static int note_cursor = 0;
static int sx[100], sy[100], slen = 3, sdir = 1, fx = 10, fy = 10;
static bool s_dead = false;
static uint64_t last_s_move = 0;

static int mine_grid[8][8];
static int mine_state[8][8];
static bool mine_gameover = false;
static bool mine_won = false;
static bool mine_first_click = true;

// Calculator state
static char calc_display[20] = "0";
static int64_t calc_a = 0;
static char calc_op = 0;
static bool calc_new = true;

// Paint state
static uint32_t paint_color = 0x000000;
static bool paint_inited = false;

static int drag_win = -1, drag_off_x = 0, drag_off_y = 0;
static int resize_win = -1, resize_start_x = 0, resize_start_y = 0,
           resize_start_w = 0, resize_start_h = 0;
static bool start_open = false;
static char start_search[32];
static int start_search_len = 0;
static int selected_icon = -1;
static int selected_file = -1;
static int last_icon = -1;
static uint64_t last_icon_tick = 0;
static int ctx_type = 0, ctx_x = 0, ctx_y = 0, ctx_file = -1;
static char dialog_title[32], dialog_msg[96];
static bool dialog_open = false;
static char file_history[8][256];
static int file_hist_count = 0;
static int file_hist_pos = -1;

static Rect dirty_rects[64];
static int num_dirty = 0;
static bool full_dirty = true;
static bool scene_dirty = true;
static uint64_t last_sc = 0;

void bring_to_front(int idx);

void mark_full() {
  full_dirty = true;
  scene_dirty = true;
}
void add_dirty(int x, int y, int w, int h) {
  if (num_dirty >= 64) {
    full_dirty = true;
    return;
  }
  dirty_rects[num_dirty++] = (Rect){x, y, w, h};
  scene_dirty = true;
}
void reset_dirty() {
  num_dirty = 0;
  full_dirty = false;
}

static uint32_t cur_save[12 * 18];
static int csx = 0, csy = 0;
static void save_cur(int x, int y) {
  csx = x;
  csy = y;
  for (int i = 0; i < 18; i++)
    for (int j = 0; j < 12; j++)
      cur_save[i * 12 + j] = fb::get_pixel_raw(x + j, y + i);
}
static void restore_cur() {
  for (int i = 0; i < 18; i++)
    for (int j = 0; j < 12; j++)
      fb::put_pixel(csx + j, csy + i, cur_save[i * 12 + j]);
}

static void draw_btn(int x, int y, int w, int h, const char *t) {
  fb::fill_rect(x, y, w, h, g::COL_GRAY);
  g::draw_raised(x, y, w, h);
  fb::draw_string(x + (w - (int)strlen(t) * 8) / 2, y + (h - 16) / 2, t,
                  g::COL_BLACK, g::COL_GRAY);
}
static bool contains(const char *text, const char *needle) {
  if (!needle[0])
    return true;
  for (int i = 0; text[i]; i++) {
    int j = 0;
    while (text[i + j] && needle[j] && text[i + j] == needle[j])
      j++;
    if (!needle[j])
      return true;
  }
  return false;
}
static void notify(const char *msg) {
  (void)msg;
}
static void open_dialog(const char *title, const char *msg) {
  strncpy(dialog_title, title, 31);
  dialog_title[31] = 0;
  strncpy(dialog_msg, msg, 95);
  dialog_msg[95] = 0;
  dialog_open = true;
  mark_full();
}
static void file_push_history() {
  if (file_hist_pos >= 0 && strcmp(file_history[file_hist_pos], fs::getcwd()) == 0)
    return;
  if (file_hist_count < 8) {
    file_hist_pos = file_hist_count++;
  } else {
    for (int i = 0; i < 7; i++)
      strcpy(file_history[i], file_history[i + 1]);
    file_hist_pos = 7;
  }
  strcpy(file_history[file_hist_pos], fs::getcwd());
}
static void file_chdir_track(const char *path) {
  if (fs::chdir(path)) {
    file_push_history();
    selected_file = -1;
    notify("Folder opened");
  } else {
    open_dialog("Files", "Folder not found");
  }
}
static void file_go_up() {
  file_chdir_track("..");
}
static void file_go_back() {
  if (file_hist_pos > 0) {
    file_hist_pos--;
    fs::chdir(file_history[file_hist_pos]);
    selected_file = -1;
    notify("Back");
  }
}
static void open_app(int idx) {
  if (idx < 0 || idx >= NUM_WIN)
    return;
  windows[idx].active = true;
  windows[idx].minimized = false;
  bring_to_front(idx);
  selected_icon = idx;
  notify(windows[idx].title);
}
static void sh_add_line(const char *line) {
  if (sh_count < 18)
    strcpy(sh_lines[sh_count++], line);
  else {
    for (int i = 0; i < 17; i++)
      strcpy(sh_lines[i], sh_lines[i + 1]);
    strcpy(sh_lines[17], line);
  }
}
static void sh_insert_char(char c) {
  int len = strlen(sh_in);
  if (len >= 63)
    return;
  for (int i = len; i >= sh_ptr; i--)
    sh_in[i + 1] = sh_in[i];
  sh_in[sh_ptr++] = c;
}
static void note_insert_char(char c) {
  if (note_len >= 1022)
    return;
  for (int i = note_len; i >= note_cursor; i--)
    notepad[i + 1] = notepad[i];
  notepad[note_cursor++] = c;
  note_len++;
}
static void note_delete_at_cursor() {
  if (note_cursor >= note_len)
    return;
  for (int i = note_cursor; i < note_len; i++)
    notepad[i] = notepad[i + 1];
  note_len--;
}
static void fill_circ(int cx, int cy, int r, uint32_t c) {
  for (int dy = -r; dy <= r; dy++)
    for (int dx = -r; dx <= r; dx++)
      if (dx * dx + dy * dy <= r * r)
        fb::put_pixel(cx + dx, cy + dy, c);
}

static uint8_t read_rtc(int reg) {
  outb(0x70, reg);
  return inb(0x71);
}
static uint8_t bcd2bin(uint8_t bcd) { return ((bcd / 16) * 10) + (bcd & 0x0F); }
static void get_rtc_time(int &h, int &m, int &s) {
  h = bcd2bin(read_rtc(0x04));
  m = bcd2bin(read_rtc(0x02));
  s = bcd2bin(read_rtc(0x00));
  h = (h + 3) % 24; // GMT+3 Local time
}

static const int cx_60[60] = {
    0,    10,  21,  31,  41,  50,  59,  67,  74,  81,  87,  91,  95,  98,  99,
    100,  99,  98,  95,  91,  87,  81,  74,  67,  59,  50,  41,  31,  21,  10,
    0,    -10, -21, -31, -41, -50, -59, -67, -74, -81, -87, -91, -95, -98, -99,
    -100, -99, -98, -95, -91, -87, -81, -74, -67, -59, -50, -41, -31, -21, -10};
static const int cy_60[60] = {
    -100, -99, -98, -95, -91, -87, -81, -74, -67, -59, -50, -41, -31, -21, -10,
    0,    10,  21,  31,  41,  50,  59,  67,  74,  81,  87,  91,  95,  98,  99,
    100,  99,  98,  95,  91,  87,  81,  74,  67,  59,  50,  41,  31,  21,  10,
    0,    -10, -21, -31, -41, -50, -59, -67, -74, -81, -87, -91, -95, -98, -99};

static void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
  int dx = x1 - x0 < 0 ? -(x1 - x0) : x1 - x0;
  int dy = y1 - y0 < 0 ? -(y1 - y0) : y1 - y0;
  int sx = x0 < x1 ? 1 : -1;
  int sy = y0 < y1 ? 1 : -1;
  int err = (dx > dy ? dx : -dy) / 2, e2;
  for (;;) {
    fb::put_pixel(x0, y0, color);
    if (x0 == x1 && y0 == y1)
      break;
    e2 = err;
    if (e2 > -dx) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dy) {
      err += dx;
      y0 += sy;
    }
  }
}

static void draw_window(Window &w, int id) {
  if (!w.active)
    return;
  fb::fill_rect(w.x, w.y, w.w, w.h, g::COL_GRAY);
  g::draw_raised(w.x, w.y, w.w, w.h);
  bool is_active = (win_order[NUM_WIN - 1] == id);
  uint32_t t_bg = is_active ? g::COL_TITLEBAR : g::COL_TITLE_IA;
  uint32_t t_fg = 0xFFFFFF;
  if (g::is_dark_theme) {
    t_bg = is_active ? 0x000040 : 0x2D2D30;
  }
  fb::fill_rect(w.x + 3, w.y + 3, w.w - 6, 24, t_bg);
  int by = w.y + 15;
  fill_circ(w.x + 15, by, 8, 0xFF5F56);
  fill_circ(w.x + 35, by, 8, 0xFFBD2E);
  fill_circ(w.x + 55, by, 8, 0x27C93F);
  int tl = strlen(w.title) * 8;
  fb::draw_string(w.x + (w.w - tl) / 2, w.y + 7, w.title, t_fg, t_bg);
  int ax = w.x + 4, ay = w.y + 30, aw = w.w - 8, ah = w.h - 34;
  if (id == 0) { // Terminal
    uint32_t bg = g::is_dark_theme ? 0x000000 : 0xFFFFFF;
    uint32_t fg = g::is_dark_theme ? 0x00FF88 : 0x000000;
    uint32_t pfg = g::is_dark_theme ? 0x00DDFF : 0x000080;
    uint32_t ifg = g::is_dark_theme ? 0xFFFFFF : 0x000000;
    g::draw_sunken(ax, ay, aw, ah);
    fb::fill_rect(ax + 1, ay + 1, aw - 2, ah - 2, bg);
    int iy = ay + 6;
    for (int i = 0; i < sh_count; i++) {
      fb::draw_string(ax + 8, iy, sh_lines[i], fg, bg);
      iy += 14;
    }
    fb::draw_string(ax + 8, iy, "root@elevn32 ~ % ", pfg, bg);
    fb::draw_string(ax + 8 + 17 * 8, iy, sh_in, ifg, bg);
    if ((timer::get_ticks() / 500) % 2 == 0)
      fb::fill_rect(ax + 8 + 17 * 8 + sh_ptr * 8, iy, 2, 14, fg);
  } else if (id == 1) { // Calculator
    g::draw_sunken(ax, ay, aw, ah);
    fb::fill_rect(ax + 1, ay + 1, aw - 2, ah - 2, g::COL_WHITE);
    // Display
    fb::fill_rect(ax + 8, ay + 8, aw - 16, 30, 0x1E1E1E);
    g::draw_sunken(ax + 8, ay + 8, aw - 16, 30);
    fb::draw_string(ax + aw - 16 - (int)strlen(calc_display) * 8, ay + 15,
                    calc_display, 0x00FF00, 0x1E1E1E);
    // Buttons 4x5
    const char *btns[] = {"C", "(", ")", "/", "7", "8", "9", "*", "4",   "5",
                          "6", "-", "1", "2", "3", "+", "0", ".", "+/-", "="};
    int bw = (aw - 24) / 4, bh = (ah - 60) / 5;
    for (int i = 0; i < 20; i++) {
      int bx2 = ax + 8 + (i % 4) * (bw + 2), by2 = ay + 44 + (i / 4) * (bh + 2);
      draw_btn(bx2, by2, bw, bh, btns[i]);
    }
  } else if (id == 2) { // Paint
    g::draw_sunken(ax, ay, aw, ah);
    fb::fill_rect(ax + 1, ay + 1, aw - 2, ah - 2, g::COL_WHITE);
    if (!paint_inited) {
      for (int i = 0; i < 280 * 160; i++)
        pbuf[i] = 0xFFFFFF;
      paint_inited = true;
    }
    // Canvas
    int cw = aw - 16, ch = ah - 40;
    if (cw > 280)
      cw = 280;
    if (ch > 160)
      ch = 160;
    for (int py2 = 0; py2 < ch; py2++)
      for (int px2 = 0; px2 < cw; px2++)
        fb::put_pixel(ax + 8 + px2, ay + 8 + py2, pbuf[py2 * 280 + px2]);
    g::draw_sunken(ax + 8, ay + 8, cw, ch);
    // Color palette
    uint32_t pal[] = {0x000000, 0xFF0000, 0x00FF00, 0x0000FF,
                      0xFFFF00, 0xFF00FF, 0x00FFFF, 0xFFFFFF};
    for (int i = 0; i < 8; i++) {
      fb::fill_rect(ax + 8 + i * 18, ay + ch + 12, 16, 16, pal[i]);
      g::draw_sunken(ax + 8 + i * 18, ay + ch + 12, 16, 16);
    }
  } else if (id == 3) { // Clock
    g::draw_sunken(ax, ay, aw, ah);
    fb::fill_rect(ax + 1, ay + 1, aw - 2, ah - 2, 0x1A1A2E);
    int h, m, s;
    get_rtc_time(h, m, s);
    int cx = ax + aw / 2, cy = ay + ah / 2;
    fill_circ(cx, cy, 60, 0x2A2A40);
    for (int i = 0; i < 60; i += 5)
      draw_line(cx + cx_60[i] * 55 / 100, cy + cy_60[i] * 55 / 100,
                cx + cx_60[i] * 58 / 100, cy + cy_60[i] * 58 / 100, 0xFFFFFF);
    int sx = cx + cx_60[s] * 55 / 100, sy = cy + cy_60[s] * 55 / 100;
    int mx = cx + cx_60[m] * 45 / 100, my = cy + cy_60[m] * 45 / 100;
    int h_idx = ((h % 12) * 5 + m / 12) % 60;
    int hx = cx + cx_60[h_idx] * 30 / 100, hy = cy + cy_60[h_idx] * 30 / 100;
    draw_line(cx, cy, hx, hy, 0xFFFFFF);
    draw_line(cx + 1, cy, hx + 1, hy, 0xFFFFFF);
    draw_line(cx, cy, mx, my, 0xAAAAAA);
    draw_line(cx, cy, sx, sy, 0xFF0000);
    fill_circ(cx, cy, 3, 0xFFFFFF);
    char tc[9];
    tc[0] = '0' + h / 10;
    tc[1] = '0' + h % 10;
    tc[2] = ':';
    tc[3] = '0' + m / 10;
    tc[4] = '0' + m % 10;
    tc[5] = ':';
    tc[6] = '0' + s / 10;
    tc[7] = '0' + s % 10;
    tc[8] = 0;
    fb::draw_string(cx - 32, cy + 70, tc, 0x00FF88, 0x1A1A2E);
  } else if (id == 4) { // Settings
    g::draw_sunken(ax, ay, aw, ah);
    fb::fill_rect(ax + 1, ay + 1, aw - 2, ah - 2, g::COL_WHITE);
    fb::draw_string(ax + 8, ay + 10, "Desktop Color:", g::COL_BLACK,
                    g::COL_WHITE);
    uint32_t ccs[] = {0x1E1E3F, 0x2A1B38, 0x1A2536, 0x2B5A5F, 0x000000};
    for (int j = 0; j < 5; j++) {
      fb::fill_rect(ax + 10 + j * 38, ay + 30, 30, 30, ccs[j]);
      g::draw_sunken(ax + 10 + j * 38, ay + 30, 30, 30);
    }
    fb::draw_string(ax + 8, ay + 72, "Theme:", g::COL_BLACK, g::COL_WHITE);
    draw_btn(ax + 8, ay + 88, 80, 24, "Light");
    draw_btn(ax + 100, ay + 88, 80, 24, "Dark");
  } else if (id == 5) { // Notepad
    g::draw_sunken(ax, ay, aw, ah);
    fb::fill_rect(ax + 1, ay + 1, aw - 2, ah - 2, g::COL_WHITE);
    int tx = ax + 8, ty = ay + 8;
    int cxr = tx, cyr = ty;
    for (int i = 0; i < note_len; i++) {
      if (i == note_cursor) {
        cxr = tx;
        cyr = ty;
      }
      if (ty + 14 >= ay + ah - 30)
        break;
      if (notepad[i] == '\n') {
        tx = ax + 8;
        ty += 14;
      } else if (tx + 8 > ax + aw - 8) {
        tx = ax + 8;
        ty += 14;
        if (ty + 14 >= ay + ah - 30)
          break;
        char s[2] = {notepad[i], 0};
        fb::draw_string(tx, ty, s, g::COL_BLACK, g::COL_WHITE);
        tx += 8;
      } else {
        char s[2] = {notepad[i], 0};
        fb::draw_string(tx, ty, s, g::COL_BLACK, g::COL_WHITE);
        tx += 8;
      }
    }
    if (note_cursor == note_len) {
      cxr = tx;
      cyr = ty;
    }
    if (ty + 14 < ay + ah - 24 && (timer::get_ticks() / 500) % 2 == 0)
      fb::fill_rect(cxr, cyr, 2, 14, g::COL_BLACK);
    draw_btn(ax + 2, ay + ah - 24, 60, 20, "Save");
    draw_btn(ax + 64, ay + ah - 24, 60, 20, "Load");
  } else if (id == 6) { // System Info
    g::draw_sunken(ax, ay, aw, ah);
    fb::fill_rect(ax + 1, ay + 1, aw - 2, ah - 2, g::COL_WHITE);
    fb::draw_string(ax + 8, ay + 10, "elevn32 OS v1.1.1", g::COL_TITLEBAR,
                    g::COL_WHITE);
    fb::draw_string(ax + 8, ay + 30, "Arch:   x86_64", g::COL_BLACK,
                    g::COL_WHITE);
    fb::draw_string(ax + 8, ay + 46, "Loader: Limine v8", g::COL_BLACK,
                    g::COL_WHITE);
    fb::draw_string(ax + 8, ay + 62, "Lang:   C++20", g::COL_BLACK,
                    g::COL_WHITE);
    char hbuf[64] = "Heap:   ";
    char nb[16];
    uint_to_str(heap::get_used() / 1024, nb);
    strcat(hbuf, nb);
    strcat(hbuf, " KB used");
    fb::draw_string(ax + 8, ay + 82, hbuf, g::COL_BLACK, g::COL_WHITE);
    char fbuf[64] = "Free:   ";
    uint_to_str(heap::get_free() / 1024, nb);
    strcat(fbuf, nb);
    strcat(fbuf, " KB");
    fb::draw_string(ax + 8, ay + 98, fbuf, g::COL_BLACK, g::COL_WHITE);
    char ubuf[64] = "Uptime: ";
    uint64_t us = timer::get_ticks() / 100;
    uint_to_str(us / 60, nb);
    strcat(ubuf, nb);
    strcat(ubuf, "m ");
    uint_to_str(us % 60, nb);
    strcat(ubuf, nb);
    strcat(ubuf, "s");
    fb::draw_string(ax + 8, ay + 118, ubuf, g::COL_BLACK, g::COL_WHITE);
  } else if (id == 7) { // Snake
    g::draw_sunken(ax, ay, aw, ah);
    fb::fill_rect(ax + 1, ay + 1, aw - 2, ah - 2, 0x0A0A0A);
    for (int i = 0; i < slen; i++)
      fb::fill_rect(ax + 4 + sx[i] * 10, ay + 4 + sy[i] * 10, 9, 9,
                    i == 0 ? 0x00DD00 : 0x00AA00);
    fb::fill_rect(ax + 4 + fx * 10, ay + 4 + fy * 10, 9, 9, 0xFF3333);
    char sbuf[16] = "Score:";
    char sn[8];
    uint_to_str(slen - 3, sn);
    strcat(sbuf, sn);
    fb::draw_string(ax + 8, ay + ah - 18, sbuf, 0x888888, 0x0A0A0A);
    if (s_dead) {
      fb::draw_string(ax + aw / 2 - 40, ay + ah / 2 - 7, "GAME OVER", 0xFF3333,
                      0x0A0A0A);
      fb::draw_string(ax + aw / 2 - 52, ay + ah / 2 + 10, "R to restart",
                      0x888888, 0x0A0A0A);
    }
  } else if (id == 8) { // Minesweeper
    g::draw_sunken(ax, ay, aw, ah);
    fb::fill_rect(ax + 1, ay + 1, aw - 2, ah - 2, g::COL_WHITE);
    fb::draw_string(ax + 8, ay + 10, "Minesweeper", g::COL_BLACK, g::COL_WHITE);
    if (mine_gameover)
      fb::draw_string(ax + 120, ay + 10, "GAME OVER", 0xFF0000, g::COL_WHITE);
    if (mine_won)
      fb::draw_string(ax + 120, ay + 10, "YOU WIN!", 0x00AA00, g::COL_WHITE);
    fb::draw_string(ax + 8, ay + 30, "R to Reset", 0x888888, g::COL_WHITE);
    for (int gy = 0; gy < 8; gy++)
      for (int gx = 0; gx < 8; gx++) {
        int bx = ax + 8 + gx * 22, by = ay + 50 + gy * 22;
        if (mine_state[gy][gx] == 0)
          draw_btn(bx, by, 20, 20, "");
        else if (mine_state[gy][gx] == 2)
          draw_btn(bx, by, 20, 20, "F");
        else {
          fb::fill_rect(bx, by, 20, 20, 0xEEEEEE);
          g::draw_sunken(bx, by, 20, 20);
          if (mine_grid[gy][gx] == 9)
            fb::fill_rect(bx + 4, by + 4, 12, 12, 0xFF0000);
          else if (mine_grid[gy][gx] > 0) {
            char nb[2] = {(char)('0' + mine_grid[gy][gx]), 0};
            fb::draw_string(bx + 6, by + 6, nb, 0x0000FF, 0xEEEEEE);
          }
        }
      }
  } else if (id == 9) { // File Explorer
    g::draw_sunken(ax, ay, aw, ah);
    fb::fill_rect(ax + 1, ay + 1, aw - 2, ah - 2, g::COL_WHITE);
    fb::fill_rect(ax + 1, ay + 1, 120, ah - 2, 0xEAEAEA); // Left panel
    fb::draw_string(ax + 10, ay + 10, "C: Disk", g::COL_BLACK, 0xEAEAEA);
    draw_btn(ax + 128, ay + 6, 42, 22, "Back");
    draw_btn(ax + 174, ay + 6, 28, 22, "Up");
    fb::fill_rect(ax + 208, ay + 6, aw - 216, 22, 0xF7F7F7);
    g::draw_sunken(ax + 208, ay + 6, aw - 216, 22);
    fb::draw_string(ax + 214, ay + 10, fs::getcwd(), g::COL_TITLEBAR,
                    0xF7F7F7);
    int fcount = fs::get_file_count();
    for (int j = 0; j < fcount; j++) {
      uint32_t color = fs::get_file_is_dir(j) ? g::COL_TITLEBAR : g::COL_BLACK;
      int row_y = ay + 38 + j * 22;
      if (j == selected_file)
        fb::fill_rect(ax + 126, row_y - 2, aw - 134, 20, 0xB8D7FF);
      fb::fill_rect(ax + 132, row_y + 1, 12, 10,
                    fs::get_file_is_dir(j) ? 0xFFA500 : 0xFFFFFF);
      g::draw_sunken(ax + 132, row_y + 1, 12, 10);
      fb::draw_string(ax + 152, row_y, fs::get_file_name(j), color,
                      g::COL_WHITE);
    }
  }
}

void bring_to_front(int idx) {
  int pos = 0;
  for (int i = 0; i < NUM_WIN; i++)
    if (win_order[i] == idx) {
      pos = i;
      break;
    }
  for (int i = pos; i < NUM_WIN - 1; i++)
    win_order[i] = win_order[i + 1];
  win_order[NUM_WIN - 1] = idx;
  mark_full();
}

void draw_scene(int mx, int my) {
  if (current_state == STATE_LOGIN) {
    fb::fill_rect(0, 0, 1280, 800, g::current_desktop_color);
    int lx = 1280 / 2 - 150, ly = 800 / 2 - 100;
    fb::fill_rect(lx, ly, 300, 200, g::COL_GRAY);
    g::draw_raised(lx, ly, 300, 200);
    fb::draw_string(lx + 80, ly + 15, "Login", g::COL_BLACK, g::COL_GRAY);
    fb::draw_string(lx + 20, ly + 60, "User:", g::COL_BLACK, g::COL_GRAY);
    fb::fill_rect(lx + 80, ly + 55, 180, 24, g::COL_WHITE);
    g::draw_sunken(lx + 80, ly + 55, 180, 24);
    fb::draw_string(lx + 84, ly + 59, l_user, g::COL_BLACK, g::COL_WHITE);
    fb::draw_string(lx + 20, ly + 100, "Pass:", g::COL_BLACK, g::COL_GRAY);
    fb::fill_rect(lx + 80, ly + 95, 180, 24, g::COL_WHITE);
    g::draw_sunken(lx + 80, ly + 95, 180, 24);
    char stars[32];
    for (int i = 0; i < l_plen && i < 31; i++)
      stars[i] = '*';
    stars[l_plen] = 0;
    fb::draw_string(lx + 84, ly + 99, stars, g::COL_BLACK, g::COL_WHITE);
    draw_btn(lx + 100, ly + 140, 100, 30, "Login");
    if (login_err)
      fb::draw_string(lx + 60, ly + 175, "Invalid Username or Password!",
                      0xFF3333, g::COL_GRAY);
    if (l_focus == 0)
      fb::fill_rect(lx + 80, ly + 78, 180, 2, g::COL_TITLEBAR);
    if (l_focus == 1)
      fb::fill_rect(lx + 80, ly + 118, 180, 2, g::COL_TITLEBAR);
    return;
  }
  if (full_dirty) {
    fb::fill_rect(0, 0, 1280, 770, g::current_desktop_color);
  } else {
    for (int i = 0; i < num_dirty; i++) {
      Rect &r = dirty_rects[i];
      if (r.y < 770) {
        int32_t fh = (r.y + r.h > 770) ? 770 - r.y : r.h;
        fb::fill_rect(r.x, r.y, r.w, fh, g::current_desktop_color);
      }
    }
  }

  const char *icon_names[] = {"Terminal",    "Calculator", "Paint",    "Clock",
                              "Settings",    "Notepad",    "Sys Info", "Snake",
                              "Minesweeper", "Files"};
  uint32_t icon_colors[] = {0x111111, 0x555555, 0xAA0000, 0x222222, 0x888888,
                            0xFFFFFF, 0x0000AA, 0x00AA00, 0x666666, 0xEEEE00};
  for (int i = 0; i < 10; i++) {
    int cx = 40, cy = 20 + i * 70;
    if (i == selected_icon) {
      fb::fill_rect(cx - 6, cy - 4, 44, 56, 0x3A6EA5);
      g::draw_sunken(cx - 6, cy - 4, 44, 56);
    }
    fb::fill_rect(cx, cy, 32, 32, icon_colors[i]);
    g::draw_raised(cx, cy, 32, 32);
    if (i == 0) {
      fb::draw_string(cx + 6, cy + 8, ">_", 0x00FF00, icon_colors[i]);
    } else if (i == 1) {
      fb::draw_string(cx + 8, cy + 8, "+-", 0xFFFFFF, icon_colors[i]);
    } else if (i == 2) {
      fb::fill_rect(cx + 6, cy + 8, 20, 16, 0xFFFFFF);
      fb::fill_rect(cx + 8, cy + 10, 4, 4, 0xFF0000);
      fb::fill_rect(cx + 14, cy + 10, 4, 4, 0x00FF00);
      fb::fill_rect(cx + 20, cy + 10, 4, 4, 0x0000FF);
    } else if (i == 3) {
      fb::fill_rect(cx + 8, cy + 8, 16, 16, 0xFFFFFF);
      fb::fill_rect(cx + 15, cy + 10, 2, 7, 0x000000);
    } else if (i == 4) {
      fb::draw_string(cx + 8, cy + 8, "{}", 0xFFFFFF, icon_colors[i]);
    } else if (i == 5) {
      fb::fill_rect(cx + 8, cy + 6, 16, 20, 0xFFFFFF);
      fb::fill_rect(cx + 10, cy + 10, 12, 2, 0x000000);
      fb::fill_rect(cx + 10, cy + 14, 12, 2, 0x000000);
    } else if (i == 6) {
      fb::draw_string(cx + 12, cy + 8, "i", 0xFFFFFF, icon_colors[i]);
    } else if (i == 7) {
      fb::fill_rect(cx + 8, cy + 10, 16, 4, 0x00FF00);
      fb::fill_rect(cx + 20, cy + 10, 4, 12, 0x00FF00);
    } else if (i == 8) {
      fb::fill_rect(cx + 10, cy + 10, 12, 12, 0x000000);
      fb::fill_rect(cx + 12, cy + 12, 4, 4, 0xFF0000);
    } else if (i == 9) {
      fb::fill_rect(cx + 6, cy + 10, 20, 14, 0xFFA500);
      fb::fill_rect(cx + 6, cy + 6, 8, 4, 0xFFA500);
    }
    int tl = strlen(icon_names[i]) * 8;
    fb::draw_string(cx + 16 - (tl / 2), cy + 36, icon_names[i], 0xFFFFFF,
                    i == selected_icon ? 0x3A6EA5 : g::current_desktop_color);
  }
  for (int i = 0; i < NUM_WIN; i++) {
    Window &w = windows[win_order[i]];
    if (!w.active || w.minimized)
      continue;
    bool idirty = full_dirty;
    if (!idirty) {
      for (int d = 0; d < num_dirty; d++) {
        Rect &r = dirty_rects[d];
        if (!(w.x > r.x + r.w || w.x + w.w < r.x || w.y > r.y + r.h ||
              w.y + w.h < r.y)) {
          idirty = true;
          break;
        }
      }
    }
    if (idirty)
      draw_window(w, win_order[i]);
  }
  bool tdirty = full_dirty || start_open;
  if (!tdirty) {
    for (int d = 0; d < num_dirty; d++)
      if (dirty_rects[d].y + dirty_rects[d].h >= 770) {
        tdirty = true;
        break;
      }
  }
  if (tdirty) {
    int tby = 800 - 30;
    fb::fill_rect(0, tby, 1280, 30, g::COL_GRAY);
    fb::fill_rect(0, tby, 1280, 2, g::COL_WHITE);
    draw_btn(2, tby + 3, 60, 24, "Start");
    int bx = 68;
    const char *nm[] = {"Term", "Calc", "Paint", "Clock", "Set",
                        "Note", "Info", "Snake", "Mine",  "Files"};
    for (int i = 0; i < NUM_WIN; i++)
      if (windows[i].active) {
        bool active_task = win_order[NUM_WIN - 1] == i && !windows[i].minimized;
        fb::fill_rect(bx, tby + 3, 54, 24, g::COL_GRAY);
        if (active_task)
          fb::fill_rect(bx + 2, tby + 5, 50, 20, g::COL_TITLEBAR);
        g::draw_sunken(bx, tby + 3, 54, 24);
        fb::draw_string(bx + 4, tby + 7, nm[i],
                        active_task ? 0xFFFFFF : g::COL_BLACK,
                        active_task ? g::COL_TITLEBAR : g::COL_GRAY);
        bx += 58;
      }
    int h, m, s;
    get_rtc_time(h, m, s);
    char tc[6];
    tc[0] = '0' + h / 10;
    tc[1] = '0' + h % 10;
    tc[2] = ':';
    tc[3] = '0' + m / 10;
    tc[4] = '0' + m % 10;
    tc[5] = 0;
    g::draw_sunken(1170, tby + 3, 100, 24);
    fb::draw_string(1175, tby + 7, "Time:", g::COL_BLACK, g::COL_GRAY);
    fb::draw_string(1220, tby + 7, tc, g::COL_BLACK, g::COL_GRAY);
    if (start_open) {
      int sx2 = 2, sy2 = tby - 260;
      fb::fill_rect(sx2, sy2, 170, 260, g::COL_GRAY);
      g::draw_raised(sx2, sy2, 170, 260);
      fb::fill_rect(sx2 + 8, sy2 + 8, 154, 22, g::COL_WHITE);
      g::draw_sunken(sx2 + 8, sy2 + 8, 154, 22);
      fb::draw_string(sx2 + 12, sy2 + 12, start_search, g::COL_BLACK,
                      g::COL_WHITE);
      const char *si[] = {"Terminal",      "Calculator", "Paint",
                          "Clock",         "Settings",   "Notepad",
                          "System Info",   "Snake Game", "Minesweeper",
                          "File Explorer", "Logout"};
      for (int i = 0; i < 11; i++) {
        if (!contains(si[i], start_search))
          continue;
        int iy = sy2 + 36 + i * 19;
        bool h =
            (mx >= sx2 + 2 && mx <= sx2 + 168 && my >= iy && my <= iy + 17);
        if (h)
          fb::fill_rect(sx2 + 2, iy, 166, 17, g::COL_TITLEBAR);
        fb::draw_string(sx2 + 8, iy + 2, si[i], h ? g::COL_WHITE : g::COL_BLACK,
                        h ? g::COL_TITLEBAR : g::COL_GRAY);
      }
    }
  }
  if (ctx_type) {
    const char *items_desktop[] = {"New Folder", "New File", "Refresh",
                                   "Settings"};
    const char *items_file[] = {"Open", "Rename", "Delete", "Properties"};
    const char **items = ctx_type == 1 ? items_desktop : items_file;
    fb::fill_rect(ctx_x, ctx_y, 120, 86, g::COL_GRAY);
    g::draw_raised(ctx_x, ctx_y, 120, 86);
    for (int i = 0; i < 4; i++) {
      int iy = ctx_y + 4 + i * 20;
      bool h = mx >= ctx_x && mx <= ctx_x + 120 && my >= iy && my <= iy + 18;
      if (h)
        fb::fill_rect(ctx_x + 2, iy, 116, 18, g::COL_TITLEBAR);
      fb::draw_string(ctx_x + 8, iy + 2, items[i],
                      h ? 0xFFFFFF : g::COL_BLACK,
                      h ? g::COL_TITLEBAR : g::COL_GRAY);
    }
  }
  if (dialog_open) {
    int dx = 430, dy = 290;
    fb::fill_rect(dx, dy, 420, 160, g::COL_GRAY);
    g::draw_raised(dx, dy, 420, 160);
    fb::fill_rect(dx + 3, dy + 3, 414, 24, g::COL_TITLEBAR);
    fb::draw_string(dx + 12, dy + 7, dialog_title, 0xFFFFFF, g::COL_TITLEBAR);
    fb::draw_string(dx + 20, dy + 58, dialog_msg, g::COL_BLACK, g::COL_GRAY);
    draw_btn(dx + 170, dy + 116, 80, 26, "OK");
  }
}

void init() {
  fs::init();
  file_push_history();
  start_search[0] = 0;
  start_search_len = 0;
  selected_icon = -1;
  selected_file = -1;
  for (int i = 0; i < NUM_WIN; i++) {
    windows[i].active = false;
    windows[i].minimized = false;
    win_order[i] = i;
  }
  windows[0] = (Window){false, false, false, 100, 60,  500,
                        320,   100,   60,    500, 320, "Terminal"};
  windows[1] = (Window){false, false, false, 350, 100, 240,
                        310,   350,   100,   240, 310, "Calculator"};
  windows[2] = (Window){false, false, false, 150, 100, 310,
                        200,   150,   100,   310, 200, "Paint"};
  windows[3] = (Window){false, false, false, 500, 60,  220,
                        220,   500,   60,    220, 220, "Clock"};
  windows[4] = (Window){false, false, false, 400, 200, 300,
                        160,   400,   200,   300, 160, "Settings"};
  windows[5] = (Window){false, false, false, 200, 150, 400,
                        280,   200,   150,   400, 280, "Notepad"};
  windows[6] = (Window){false, false, false, 80,  80,  320,
                        180,   80,    80,    320, 180, "System Info"};
  windows[7] = (Window){false, false, false, 450, 150, 250,
                        250,   450,   150,   250, 250, "Snake"};
  windows[8] = (Window){false, false, false, 100, 100, 300,
                        300,   100,   100,   300, 300, "Minesweeper"};
  windows[9] = (Window){false, false, false, 200, 80,  450,
                        300,   200,   80,    450, 300, "File Explorer"};
  for (int y = 0; y < 8; y++)
    for (int x = 0; x < 8; x++) {
      mine_grid[y][x] = 0;
      mine_state[y][x] = 0;
    }
  mine_gameover = false;
  mine_won = false;
  mine_first_click = true;
  mark_full();
}

void run() {
  static bool prev_left = false;
  static bool prev_right = false;
  static int pmx = 0, pmy = 0;
  while (true) {
    char c = 0;
    if (keyboard::has_input())
      c = keyboard::get_char();
    keyboard::SpecialKey sk = keyboard::get_special_key();
    if (current_state == STATE_DESKTOP && sk != keyboard::KEY_NONE) {
      if (sk == keyboard::KEY_ESCAPE) {
        start_open = false;
        ctx_type = 0;
        dialog_open = false;
        mark_full();
      } else if (keyboard::alt_down() && sk == keyboard::KEY_RIGHT) {
        for (int s = 1; s <= NUM_WIN; s++) {
          int idx = (win_order[NUM_WIN - 1] + s) % NUM_WIN;
          if (windows[idx].active && !windows[idx].minimized) {
            bring_to_front(idx);
            break;
          }
        }
      } else {
        int tw = win_order[NUM_WIN - 1];
        if (tw == 0 && windows[0].active) {
          int len = strlen(sh_in);
          if (sk == keyboard::KEY_LEFT && sh_ptr > 0)
            sh_ptr--;
          else if (sk == keyboard::KEY_RIGHT && sh_ptr < len)
            sh_ptr++;
          else if (sk == keyboard::KEY_UP && sh_hist_count > 0) {
            if (sh_hist_pos < 0)
              sh_hist_pos = sh_hist_count - 1;
            else if (sh_hist_pos > 0)
              sh_hist_pos--;
            strcpy(sh_in, sh_hist[sh_hist_pos]);
            sh_ptr = strlen(sh_in);
          } else if (sk == keyboard::KEY_DOWN && sh_hist_count > 0) {
            if (sh_hist_pos >= 0 && sh_hist_pos < sh_hist_count - 1) {
              sh_hist_pos++;
              strcpy(sh_in, sh_hist[sh_hist_pos]);
              sh_ptr = strlen(sh_in);
            } else {
              sh_hist_pos = -1;
              sh_in[0] = 0;
              sh_ptr = 0;
            }
          } else if (sk == keyboard::KEY_DELETE && sh_ptr < len) {
            for (int i = sh_ptr; i < len; i++)
              sh_in[i] = sh_in[i + 1];
          }
          add_dirty(windows[0].x, windows[0].y, windows[0].w, windows[0].h);
        } else if (tw == 5 && windows[5].active) {
          if (sk == keyboard::KEY_LEFT && note_cursor > 0)
            note_cursor--;
          else if (sk == keyboard::KEY_RIGHT && note_cursor < note_len)
            note_cursor++;
          else if (sk == keyboard::KEY_DELETE)
            note_delete_at_cursor();
          add_dirty(windows[5].x, windows[5].y, windows[5].w, windows[5].h);
        }
      }
    }
    if (c) {
      if (current_state == STATE_LOGIN) {
        if (c == '\t') {
          l_focus = l_focus ? 0 : 1;
        } else if (c == '\b') {
          if (l_focus == 0 && l_ulen > 0)
            l_user[--l_ulen] = 0;
          if (l_focus == 1 && l_plen > 0)
            l_pass[--l_plen] = 0;
          login_err = false;
        } else if (c == '\n') {
          if (strcmp(l_user, "root") == 0 && strcmp(l_pass, "123") == 0) {
            current_state = STATE_DESKTOP;
            login_err = false;
            mark_full();
          } else {
            login_err = true;
            mark_full();
          }
        } else {
          if (l_focus == 0 && l_ulen < 31) {
            l_user[l_ulen++] = c;
            l_user[l_ulen] = 0;
            login_err = false;
          }
          if (l_focus == 1 && l_plen < 31) {
            l_pass[l_plen++] = c;
            l_pass[l_plen] = 0;
            login_err = false;
          }
        }
        mark_full();
      } else {
        if (keyboard::alt_down() && c == '\t') {
          for (int p = NUM_WIN - 2; p >= 0; p--) {
            int idx = win_order[p];
            if (windows[idx].active && !windows[idx].minimized) {
              bring_to_front(idx);
              break;
            }
          }
          mark_full();
          continue;
        }
        if (start_open) {
          if (c == '\b') {
            if (start_search_len > 0)
              start_search[--start_search_len] = 0;
          } else if (c == '\n') {
            const char *si[] = {"Terminal",      "Calculator", "Paint",
                                "Clock",         "Settings",   "Notepad",
                                "System Info",   "Snake Game", "Minesweeper",
                                "File Explorer", "Logout"};
            for (int i = 0; i < 11; i++)
              if (contains(si[i], start_search)) {
                if (i == 10)
                  current_state = STATE_LOGIN;
                else
                  open_app(i);
                break;
              }
            start_open = false;
            start_search[0] = 0;
            start_search_len = 0;
          } else if (start_search_len < 31 && c >= 32) {
            start_search[start_search_len++] = c;
            start_search[start_search_len] = 0;
          }
          mark_full();
          continue;
        }
        if (keyboard::ctrl_down() && c == 'l') {
          start_open = true;
          start_search[0] = 0;
          start_search_len = 0;
          mark_full();
          continue;
        }
        if (keyboard::ctrl_down() && c == 'n') {
          if (fs::mkdir("NewFolder"))
            notify("Folder created");
          else if (fs::touch("NewFile.txt"))
            notify("File created");
          else
            open_dialog("Create", "Could not create item");
          mark_full();
          continue;
        }
        int tw = win_order[NUM_WIN - 1];
        if (tw == 0 && windows[0].active) {
          if (c == '\b') {
            if (sh_ptr > 0) {
              int len = strlen(sh_in);
              for (int i = sh_ptr - 1; i < len; i++)
                sh_in[i] = sh_in[i + 1];
              sh_ptr--;
            }
          } else if (c == '\n') {
            char prompt[80] = "~ % ";
            strcat(prompt, sh_in);
            sh_add_line(prompt);
            if (strlen(sh_in) > 0) {
              if (sh_hist_count < 8)
                strcpy(sh_hist[sh_hist_count++], sh_in);
              else {
                for (int i = 0; i < 7; i++)
                  strcpy(sh_hist[i], sh_hist[i + 1]);
                strcpy(sh_hist[7], sh_in);
              }
            }
            sh_hist_pos = -1;
            shell::process_command_wm(sh_in, sh_lines, &sh_count);
            sh_in[0] = 0;
            sh_ptr = 0;
          } else if (c >= 32) {
            sh_insert_char(c);
          }
          add_dirty(windows[0].x, windows[0].y, windows[0].w, windows[0].h);
        } else if (tw == 5 && windows[5].active) {
          if (c == '\b') {
            if (note_cursor > 0) {
              note_cursor--;
              note_delete_at_cursor();
            }
          } else {
            note_insert_char(c);
          }
          add_dirty(windows[5].x, windows[5].y, windows[5].w, windows[5].h);
        } else if (tw == 7 && windows[7].active) {
          if (c == 'w')
            sdir = 0;
          else if (c == 'd')
            sdir = 1;
          else if (c == 's')
            sdir = 2;
          else if (c == 'a')
            sdir = 3;
          else if ((c == 'r' || c == 'R') && s_dead) {
            s_dead = false;
            slen = 3;
            sdir = 1;
            fx = 10;
            fy = 10;
            sx[0] = 5;
            sy[0] = 5;
            sx[1] = 4;
            sy[1] = 5;
            sx[2] = 3;
            sy[2] = 5;
          }
        } else if (tw == 8 && windows[8].active) {
          if (c == 'r' || c == 'R') {
            for (int y = 0; y < 8; y++)
              for (int x = 0; x < 8; x++) {
                mine_grid[y][x] = 0;
                mine_state[y][x] = 0;
              }
            mine_gameover = false;
            mine_won = false;
            mine_first_click = true;
            add_dirty(windows[8].x, windows[8].y, windows[8].w, windows[8].h);
          }
        }
      }
    }
    int mx = mouse::get_x(), my = mouse::get_y();
    bool ml = mouse::is_left_clicked();
    bool mr = mouse::is_right_clicked();
    bool clicked = (ml && !prev_left);
    bool right_clicked = (mr && !prev_right);
    uint64_t now_ms = timer::get_ticks();
    if (current_state == STATE_DESKTOP && windows[7].active && !s_dead &&
        now_ms - last_s_move > 100) {
      last_s_move = now_ms;
      for (int i = slen - 1; i > 0; i--) {
        sx[i] = sx[i - 1];
        sy[i] = sy[i - 1];
      }
      if (sdir == 0)
        sy[0]--;
      else if (sdir == 1)
        sx[0]++;
      else if (sdir == 2)
        sy[0]++;
      else if (sdir == 3)
        sx[0]--;
      int mw = (windows[7].w - 8) / 10, mh = (windows[7].h - 28 - 8) / 10;
      if (sx[0] < 0 || sy[0] < 0 || sx[0] >= mw || sy[0] >= mh)
        s_dead = true;
      for (int i = 1; i < slen; i++)
        if (sx[0] == sx[i] && sy[0] == sy[i])
          s_dead = true;
      if (sx[0] == fx && sy[0] == fy) {
        if (slen < 99)
          slen++;
        fx = (fx * 7 + 13) % mw;
        fy = (fy * 5 + 11) % mh;
      }
      add_dirty(windows[7].x, windows[7].y, windows[7].w, windows[7].h);
    }
    if (right_clicked && current_state == STATE_DESKTOP) {
      ctx_type = 1;
      ctx_x = mx;
      ctx_y = my;
      ctx_file = -1;
      if (windows[9].active && !windows[9].minimized &&
          mx >= windows[9].x && mx <= windows[9].x + windows[9].w &&
          my >= windows[9].y && my <= windows[9].y + windows[9].h) {
        Window &fw = windows[9];
        int ax2 = fw.x + 4, ay2 = fw.y + 30;
        int fcount = fs::get_file_count();
        for (int j = 0; j < fcount; j++) {
          if (mx >= ax2 + 126 && mx <= ax2 + fw.w - 12 &&
              my >= ay2 + 38 + j * 22 && my <= ay2 + 58 + j * 22) {
            ctx_type = 2;
            ctx_file = j;
            selected_file = j;
            break;
          }
        }
      }
      mark_full();
    }
    if (clicked) {
      if (current_state == STATE_LOGIN) {
        int lx = 1280 / 2 - 150, ly = 800 / 2 - 100;
        if (mx >= lx + 80 && mx <= lx + 260 && my >= ly + 55 && my <= ly + 79)
          l_focus = 0;
        if (mx >= lx + 80 && mx <= lx + 260 && my >= ly + 95 && my <= ly + 119)
          l_focus = 1;
        if (mx >= lx + 100 && mx <= lx + 200 && my >= ly + 140 &&
            my <= ly + 170) {
          if (strcmp(l_user, "root") == 0 && strcmp(l_pass, "123") == 0) {
            current_state = STATE_DESKTOP;
            login_err = false;
            mark_full();
          } else {
            login_err = true;
            mark_full();
          }
        }
        mark_full();
      } else {
        if (dialog_open) {
          int dx = 430, dy = 290;
          if (mx >= dx + 170 && mx <= dx + 250 && my >= dy + 116 &&
              my <= dy + 142)
            dialog_open = false;
          mark_full();
          prev_left = ml;
          prev_right = mr;
          continue;
        }
        if (ctx_type) {
          if (mx >= ctx_x && mx <= ctx_x + 120 && my >= ctx_y &&
              my <= ctx_y + 86) {
            int item = (my - ctx_y - 4) / 20;
            if (item >= 0 && item < 4) {
              if (ctx_type == 1) {
                if (item == 0) {
                  if (fs::mkdir("NewFolder"))
                    notify("Folder created");
                  else
                    open_dialog("New Folder", "Could not create folder");
                } else if (item == 1) {
                  if (fs::touch("NewFile.txt"))
                    notify("File created");
                  else
                    open_dialog("New File", "Could not create file");
                } else if (item == 2) {
                  notify("Desktop refreshed");
                } else if (item == 3) {
                  open_app(4);
                }
              } else if (ctx_type == 2 && ctx_file >= 0) {
                const char *fname = fs::get_file_name(ctx_file);
                if (item == 0) {
                  if (fs::get_file_is_dir(ctx_file))
                    file_chdir_track(fname);
                  else {
                    int sz;
                    fs::load_file(fname, notepad, &sz);
                    note_len = sz;
                    note_cursor = note_len;
                    open_app(5);
                  }
                } else if (item == 1) {
                  char nn[32] = "Renamed";
                  if (!fs::get_file_is_dir(ctx_file))
                    strcpy(nn, "Renamed.txt");
                  if (fs::rename(fname, nn))
                    notify("Item renamed");
                  else
                    open_dialog("Rename", "Could not rename item");
                } else if (item == 2) {
                  if (fs::remove(fname))
                    notify("Item deleted");
                  else
                    open_dialog("Delete", "Folder must be empty");
                } else if (item == 3) {
                  open_dialog("Properties",
                              fs::get_file_is_dir(ctx_file) ? "Type: folder"
                                                            : "Type: file");
                }
              }
            }
          }
          ctx_type = 0;
          mark_full();
          prev_left = ml;
          prev_right = mr;
          continue;
        }
        int tby = 800 - 30;
        if (mx >= 2 && mx <= 62 && my >= tby + 3 && my <= tby + 27) {
          start_open = !start_open;
          start_search[0] = 0;
          start_search_len = 0;
          mark_full();
        } else if (start_open) {
          int sx2 = 2, sy2 = tby - 260;
          if (mx >= sx2 && mx <= sx2 + 170 && my >= sy2 && my <= sy2 + 260) {
            bool clicked_item = false;
            const char *si[] = {"Terminal",      "Calculator", "Paint",
                                "Clock",         "Settings",   "Notepad",
                                "System Info",   "Snake Game", "Minesweeper",
                                "File Explorer", "Logout"};
            for (int i = 0; i < 11; i++)
              if (contains(si[i], start_search) &&
                  my >= sy2 + 36 + i * 19 && my <= sy2 + 36 + i * 19 + 17) {
                if (i == 10)
                  current_state = STATE_LOGIN;
                else
                  open_app(i);
                start_open = false;
                mark_full();
                clicked_item = true;
                break;
              }
            if (!clicked_item) {
              start_open = false;
              mark_full();
            }
          } else {
            start_open = false;
            mark_full();
          }
        } else {
          int bx = 68;
          bool tb_c = false;
          for (int i = 0; i < NUM_WIN; i++)
            if (windows[i].active) {
              if (mx >= bx && mx <= bx + 54 && my >= tby + 3 &&
                  my <= tby + 27) {
                windows[i].minimized = false;
                bring_to_front(i);
                tb_c = true;
                break;
              }
              bx += 58;
            }
          if (!tb_c) {
            bool icon_clicked = false;
            for (int i = 0; i < 10; i++) {
              int cx = 40, cy = 20 + i * 70;
              if (mx >= cx && mx <= cx + 32 && my >= cy && my <= cy + 32) {
                if (selected_icon == i && last_icon == i &&
                    now_ms - last_icon_tick < 40) {
                  open_app(i);
                } else {
                  selected_icon = i;
                  last_icon = i;
                  last_icon_tick = now_ms;
                }
                icon_clicked = true;
                mark_full();
                break;
              }
            }
            if (!icon_clicked) {
              for (int i = NUM_WIN - 1; i >= 0; i--) {
                int idx = win_order[i];
                Window &w = windows[idx];
                if (!w.active || w.minimized)
                  continue;
                if (mx >= w.x && mx <= w.x + w.w && my >= w.y &&
                    my <= w.y + w.h) {
                  bring_to_front(idx);
                  if (mx >= w.x + 7 && mx <= w.x + 23 && my >= w.y + 7 &&
                      my <= w.y + 23)
                    w.active = false;
                  else if (mx >= w.x + 27 && mx <= w.x + 43 && my >= w.y + 7 &&
                           my <= w.y + 23)
                    w.minimized = true;
                  else if (mx >= w.x + 47 && mx <= w.x + 63 && my >= w.y + 7 &&
                           my <= w.y + 23) {
                    if (w.maximized) {
                      w.x = w.rx;
                      w.y = w.ry;
                      w.w = w.rw;
                      w.h = w.rh;
                      w.maximized = false;
                    } else {
                      w.rx = w.x;
                      w.ry = w.y;
                      w.rw = w.w;
                      w.rh = w.h;
                      w.x = 0;
                      w.y = 0;
                      w.w = 1280;
                      w.h = 770;
                      w.maximized = true;
                    }
                  } else if (mx >= w.x + 66 && mx <= w.x + w.w - 3 &&
                             my >= w.y + 3 && my <= w.y + 27 && !w.maximized) {
                    drag_win = idx;
                    drag_off_x = mx - w.x;
                    drag_off_y = my - w.y;
                  } else if (!w.maximized &&
                             mx >= w.x + w.w - 10 && mx <= w.x + w.w &&
                             my >= w.y + w.h - 10 && my <= w.y + w.h) {
                    resize_win = idx;
                    resize_start_x = mx;
                    resize_start_y = my;
                    resize_start_w = w.w;
                    resize_start_h = w.h;
                  } else if (idx == 1) { // Calculator clicks
                    int ax = w.x + 4, ay = w.y + 24, aw = w.w - 8,
                        ah = w.h - 28;
                    int bw = (aw - 24) / 4, bh = (ah - 60) / 5;
                    const char *btns[] = {"C", "(", ")", "/", "7",   "8", "9",
                                          "*", "4", "5", "6", "-",   "1", "2",
                                          "3", "+", "0", ".", "+/-", "="};
                    for (int j = 0; j < 20; j++) {
                      int bx2 = ax + 8 + (j % 4) * (bw + 2),
                          by2 = ay + 44 + (j / 4) * (bh + 2);
                      if (mx >= bx2 && mx <= bx2 + bw && my >= by2 &&
                          my <= by2 + bh) {
                        if (btns[j][0] >= '0' && btns[j][0] <= '9') {
                          if (calc_new) {
                            calc_display[0] = btns[j][0];
                            calc_display[1] = 0;
                            calc_new = false;
                          } else if (strlen(calc_display) < 18) {
                            int l = strlen(calc_display);
                            calc_display[l] = btns[j][0];
                            calc_display[l + 1] = 0;
                          }
                        } else if (btns[j][0] == 'C') {
                          calc_display[0] = '0';
                          calc_display[1] = 0;
                          calc_new = true;
                          calc_a = 0;
                          calc_op = 0;
                        } else if (btns[j][0] == '+' || btns[j][0] == '-' ||
                                   btns[j][0] == '*' || btns[j][0] == '/') {
                          // Very basic: just store first operand
                          int64_t val = 0;
                          for (int k = 0; calc_display[k]; k++)
                            val = val * 10 + (calc_display[k] - '0');
                          calc_a = val;
                          calc_op = btns[j][0];
                          calc_new = true;
                        } else if (btns[j][0] == '=') {
                          int64_t val = 0;
                          for (int k = 0; calc_display[k]; k++)
                            val = val * 10 + (calc_display[k] - '0');
                          int64_t res = 0;
                          if (calc_op == '+')
                            res = calc_a + val;
                          else if (calc_op == '-')
                            res = calc_a - val;
                          else if (calc_op == '*')
                            res = calc_a * val;
                          else if (calc_op == '/')
                            res = val != 0 ? calc_a / val : 0;
                          else
                            res = val;
                          if (res < 0) {
                            calc_display[0] = '-';
                            uint_to_str(-res, calc_display + 1);
                          } else
                            uint_to_str(res, calc_display);
                          calc_new = true;
                          calc_op = 0;
                        }
                        break;
                      }
                    }
                  } else if (idx == 4) {
                    int ax2 = w.x + 4, ay2 = w.y + 24;
                    uint32_t ccs[] = {0x1E1E3F, 0x2A1B38, 0x1A2536, 0x2B5A5F,
                                      0x000000};
                    for (int j = 0; j < 5; j++)
                      if (mx >= ax2 + 10 + j * 38 && mx <= ax2 + 40 + j * 38 &&
                          my >= ay2 + 30 && my <= ay2 + 60) {
                        g::current_desktop_color = ccs[j];
                        notify("Accent color changed");
                        mark_full();
                      }
                    if (mx >= ax2 + 8 && mx <= ax2 + 88 && my >= ay2 + 88 &&
                        my <= ay2 + 112) {
                      g::COL_GRAY = 0xC0C0C0;
                      g::COL_WHITE = 0xFFFFFF;
                      g::COL_BLACK = 0x000000;
                      g::is_dark_theme = false;
                      notify("Light theme");
                      mark_full();
                    }
                    if (mx >= ax2 + 100 && mx <= ax2 + 180 && my >= ay2 + 88 &&
                        my <= ay2 + 112) {
                      g::COL_GRAY = 0x2D2D30;
                      g::COL_WHITE = 0x1E1E1E;
                      g::COL_BLACK = 0xFFFFFF;
                      g::is_dark_theme = true;
                      notify("Dark theme");
                      mark_full();
                    }
                  } else if (idx == 5) {
                    int ax2 = w.x + 4, ay2 = w.y + 24, ah2 = w.h - 28;
                    if (mx >= ax2 + 2 && mx <= ax2 + 62 &&
                        my >= ay2 + ah2 - 24 && my <= ay2 + ah2 - 4) {
                      fs::save_file("note.txt", notepad, note_len);
                      notify("File saved");
                    }
                    if (mx >= ax2 + 64 && mx <= ax2 + 124 &&
                        my >= ay2 + ah2 - 24 && my <= ay2 + ah2 - 4) {
                      int sz;
                      if (fs::load_file("note.txt", notepad, &sz)) {
                        note_len = sz;
                        note_cursor = note_len;
                        notify("File loaded");
                      } else {
                        open_dialog("Notepad", "note.txt was not found");
                      }
                    }
                  } else if (idx == 8) {
                    int ax2 = w.x + 4, ay2 = w.y + 24;
                    for (int gy = 0; gy < 8; gy++)
                      for (int gx = 0; gx < 8; gx++) {
                        int bx = ax2 + 8 + gx * 22, by = ay2 + 50 + gy * 22;
                        if (mx >= bx && mx <= bx + 20 && my >= by &&
                            my <= by + 20 && !mine_gameover && !mine_won) {
                          if (mine_first_click) {
                            mine_first_click = false;
                            int placed = 0;
                            uint64_t seed = timer::get_ticks() + mx + my;
                            while (placed < 10) {
                              int pos = seed % 64;
                              seed = (seed * 13 + 17);
                              int px = pos % 8, py = pos / 8;
                              if ((px != gx || py != gy) &&
                                  mine_grid[py][px] != 9) {
                                mine_grid[py][px] = 9;
                                placed++;
                              }
                            }
                            for (int y = 0; y < 8; y++)
                              for (int x = 0; x < 8; x++) {
                                if (mine_grid[y][x] == 9)
                                  continue;
                                int c = 0;
                                for (int dy = -1; dy <= 1; dy++)
                                  for (int dx = -1; dx <= 1; dx++) {
                                    if (y + dy >= 0 && y + dy < 8 &&
                                        x + dx >= 0 && x + dx < 8 &&
                                        mine_grid[y + dy][x + dx] == 9)
                                      c++;
                                  }
                                mine_grid[y][x] = c;
                              }
                          }
                          if (mine_state[gy][gx] == 0) {
                            mine_state[gy][gx] = 1;
                            if (mine_grid[gy][gx] == 9)
                              mine_gameover = true;
                            else {
                              int hidden = 0;
                              for (int y = 0; y < 8; y++)
                                for (int x = 0; x < 8; x++)
                                  if (mine_state[y][x] == 0 ||
                                      mine_state[y][x] == 2)
                                    hidden++;
                              if (hidden == 10)
                                mine_won = true;
                            }
                          }
                          mark_full();
                          break;
                        }
                      }
                  } else if (idx == 9) {
                    int ax2 = w.x + 4, ay2 = w.y + 30;
                    if (mx >= ax2 + 128 && mx <= ax2 + 170 &&
                        my >= ay2 + 6 && my <= ay2 + 28) {
                      file_go_back();
                      mark_full();
                      break;
                    }
                    if (mx >= ax2 + 174 && mx <= ax2 + 202 &&
                        my >= ay2 + 6 && my <= ay2 + 28) {
                      file_go_up();
                      mark_full();
                      break;
                    }
                    int fcount = fs::get_file_count();
                    for (int j = 0; j < fcount; j++) {
                      if (mx >= ax2 + 126 && mx <= ax2 + w.w - 12 &&
                          my >= ay2 + 38 + j * 22 && my <= ay2 + 58 + j * 22) {
                        const char *fname = fs::get_file_name(j);
                        if (selected_file == j && last_icon == 100 + j &&
                            now_ms - last_icon_tick < 40) {
                          if (fs::get_file_is_dir(j)) {
                            file_chdir_track(fname);
                          } else {
                            int sz;
                            fs::load_file(fname, notepad, &sz);
                            note_len = sz;
                            note_cursor = note_len;
                            open_app(5);
                          }
                        } else {
                          selected_file = j;
                          last_icon = 100 + j;
                          last_icon_tick = now_ms;
                        }
                        mark_full();
                        break;
                      }
                    }
                  }
                  mark_full();
                  break;
                }
              }
            }
          }
        }
      }
    } else if (ml && win_order[NUM_WIN - 1] == 2 &&
               windows[2].active) { // Paint drag drawing
      Window &w = windows[2];
      int ax = w.x + 4, ay = w.y + 24, aw = w.w - 8, ah = w.h - 28;
      int cw = aw - 16, ch = ah - 40;
      if (cw > 280)
        cw = 280;
      if (ch > 160)
        ch = 160;
      int cx = mx - (ax + 8), cy = my - (ay + 8);
      if (cx >= 0 && cx < cw && cy >= 0 && cy < ch) {
        // Brush size 3x3
        for (int dy = -1; dy <= 1; dy++)
          for (int dx = -1; dx <= 1; dx++) {
            if (cx + dx >= 0 && cx + dx < cw && cy + dy >= 0 && cy + dy < ch)
              pbuf[(cy + dy) * 280 + (cx + dx)] = paint_color;
          }
        add_dirty(w.x, w.y, w.w, w.h);
      } else if (clicked) {
        // Palette click
        uint32_t pal[] = {0x000000, 0xFF0000, 0x00FF00, 0x0000FF,
                          0xFFFF00, 0xFF00FF, 0x00FFFF, 0xFFFFFF};
        for (int i = 0; i < 8; i++)
          if (mx >= ax + 8 + i * 18 && mx <= ax + 24 + i * 18 &&
              my >= ay + ch + 12 && my <= ay + ch + 28) {
            paint_color = pal[i];
            break;
          }
      }
    }
    if (drag_win != -1) {
      int nx = mx - drag_off_x, ny = my - drag_off_y;
      if (nx != windows[drag_win].x || ny != windows[drag_win].y) {
        add_dirty(windows[drag_win].x, windows[drag_win].y, windows[drag_win].w,
                  windows[drag_win].h);
        windows[drag_win].x = nx;
        windows[drag_win].y = ny;
        add_dirty(nx, ny, windows[drag_win].w, windows[drag_win].h);
      }
    }
    if (resize_win != -1) {
      int nw = resize_start_w + (mx - resize_start_x);
      int nh = resize_start_h + (my - resize_start_y);
      if (nw < 180)
        nw = 180;
      if (nh < 120)
        nh = 120;
      if (windows[resize_win].x + nw > 1280)
        nw = 1280 - windows[resize_win].x;
      if (windows[resize_win].y + nh > 770)
        nh = 770 - windows[resize_win].y;
      if (nw != windows[resize_win].w || nh != windows[resize_win].h) {
        add_dirty(windows[resize_win].x, windows[resize_win].y,
                  windows[resize_win].w, windows[resize_win].h);
        windows[resize_win].w = nw;
        windows[resize_win].h = nh;
        add_dirty(windows[resize_win].x, windows[resize_win].y, nw, nh);
      }
    }
    if (!ml && drag_win != -1) {
      drag_win = -1;
      mark_full();
    }
    if (!ml && resize_win != -1) {
      resize_win = -1;
      mark_full();
    }
    prev_left = ml;
    prev_right = mr;
    uint64_t now = timer::get_ticks();
    if (scene_dirty && now - last_sc >= 16) {
      draw_scene(mx, my);
      save_cur(mx, my);
      g::draw_cursor(mx, my);
      if (full_dirty)
        fb::swap_buffers();
      else
        for (int i = 0; i < num_dirty; i++)
          fb::swap_buffers_rect(dirty_rects[i].x, dirty_rects[i].y,
                                dirty_rects[i].w, dirty_rects[i].h);
      reset_dirty();
      scene_dirty = false;
      last_sc = now;
      pmx = mx;
      pmy = my;
    } else if (mx != pmx || my != pmy) {
      if (start_open)
        add_dirty(2, 800 - 30 - 260, 170, 260);
      restore_cur();
      fb::swap_buffers_rect(csx, csy, 12, 18);
      save_cur(mx, my);
      g::draw_cursor(mx, my);
      fb::swap_buffers_rect(mx, my, 12, 18);
      pmx = mx;
      pmy = my;
    }
  }
}
} // namespace wm
