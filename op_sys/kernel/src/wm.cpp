#include "wm.h"
#include "framebuffer.h"
#include "graphics.h"
#include "io.h"
#include "keyboard.h"
#include "mouse.h"
#include "string.h"
#include "timer.h"
#include "fs.h"

namespace wm {
namespace g = graphics;

enum SysState { STATE_LOGIN, STATE_DESKTOP };
static SysState current_state = STATE_LOGIN;
static char l_user[32]="root", l_pass[32]="";
static int l_ulen=4, l_plen=0, l_focus=1;

struct Window { bool active; bool minimized; bool maximized; int32_t x, y, w, h; int32_t rx, ry, rw, rh; const char *title; };
struct Rect { int32_t x, y, w, h; };
constexpr int NUM_WIN = 9;
static Window windows[NUM_WIN];
static int win_order[NUM_WIN];
static uint32_t pbuf[280*160];
static int sh_count = 0;
static char sh_lines[16][64];
static char sh_in[64];
static int sh_ptr = 0;
static char notepad[1024];
static int note_len = 0;
static int sx[100], sy[100], slen=3, sdir=1, fx=10, fy=10;
static bool s_dead=false;
static uint64_t last_s_move = 0;

static int drag_win = -1, drag_off_x=0, drag_off_y=0, drag_target_x=0, drag_target_y=0;
static bool start_open = false;

static Rect dirty_rects[64];
static int num_dirty = 0;
static bool full_dirty = true;
static bool scene_dirty = true;
static uint64_t last_sc = 0;

void mark_full() { full_dirty = true; scene_dirty = true; }
void add_dirty(int x, int y, int w, int h) {
  if (num_dirty >= 64) { full_dirty = true; return; }
  dirty_rects[num_dirty++] = (Rect){x, y, w, h};
  scene_dirty = true;
}
void reset_dirty() { num_dirty = 0; full_dirty = false; }

static uint32_t cur_save[12*18];
static int csx=0, csy=0;
static void save_cur(int x, int y) { csx=x; csy=y; for(int i=0;i<18;i++) for(int j=0;j<12;j++) cur_save[i*12+j]=fb::get_pixel_raw(x+j, y+i); }
static void restore_cur() { for(int i=0;i<18;i++) for(int j=0;j<12;j++) fb::put_pixel(csx+j, csy+i, cur_save[i*12+j]); }

static void draw_btn(int x, int y, int w, int h, const char *t) {
  fb::fill_rect(x, y, w, h, g::COL_GRAY); g::draw_raised(x, y, w, h);
  fb::draw_string(x+(w-(int)strlen(t)*8)/2, y+(h-16)/2, t, g::COL_BLACK, g::COL_GRAY);
}
static void fill_circ(int cx, int cy, int r, uint32_t c) {
  for(int dy=-r;dy<=r;dy++) for(int dx=-r;dx<=r;dx++) if(dx*dx+dy*dy<=r*r) fb::put_pixel(cx+dx,cy+dy,c);
}

static void draw_window(Window &w, int id) {
  if (!w.active) return;
  fb::fill_rect(w.x, w.y, w.w, w.h, g::COL_GRAY); g::draw_raised(w.x, w.y, w.w, w.h);
  uint32_t t_bg = (g::COL_GRAY == 0x2D2D30) ? g::COL_GRAY : 0xFFFFFF;
  uint32_t t_fg = (g::COL_GRAY == 0x2D2D30) ? 0xFFFFFF : 0x000000;
  fb::fill_rect(w.x+3, w.y+3, w.w-6, 18, t_bg);
  int by = w.y+12; 
  fill_circ(w.x+12, by, 5, 0xFF5F56); fill_circ(w.x+26, by, 5, 0xFFBD2E); fill_circ(w.x+40, by, 5, 0x27C93F);
  int tl = strlen(w.title)*8; fb::draw_string(w.x+(w.w-tl)/2, w.y+4, w.title, t_fg, t_bg);
  int ax = w.x+4, ay = w.y+24, aw = w.w-8, ah = w.h-28;
  if (id == 0) { // Terminal
    g::draw_sunken(ax, ay, aw, ah); fb::fill_rect(ax+1, ay+1, aw-2, ah-2, g::COL_WHITE);
    int ml = (ah-16)/14; int st = sh_count>ml?sh_count-ml:0; int iy = ay+6;
    for (int i=st; i<sh_count; i++) { fb::draw_string(ax+8, iy, sh_lines[i], g::COL_BLACK, g::COL_WHITE); iy+=14; }
    fb::draw_string(ax+8, iy, "root@elevn32 ~ % ", g::COL_BLACK, g::COL_WHITE);
    fb::draw_string(ax+8+17*8, iy, sh_in, g::COL_BLACK, g::COL_WHITE);
  } else if (id == 5) { // Notepad
    g::draw_sunken(ax, ay, aw, ah); fb::fill_rect(ax+1, ay+1, aw-2, ah-2, g::COL_WHITE);
    int tx=ax+8, ty=ay+8;
    for(int i=0; i<note_len; i++) { if (notepad[i]=='\n') { tx=ax+8; ty+=14; } else { char s[2]={notepad[i],0}; fb::draw_string(tx, ty, s, g::COL_BLACK, g::COL_WHITE); tx+=8; } }
    if ((timer::get_ticks()/500)%2==0) fb::fill_rect(tx, ty, 2, 14, g::COL_BLACK);
    draw_btn(ax+2, ay+ah-24, 60, 20, "Save"); draw_btn(ax+64, ay+ah-24, 60, 20, "Load");
  } else if (id == 7) { // Snake
    g::draw_sunken(ax, ay, aw, ah); fb::fill_rect(ax+1, ay+1, aw-2, ah-2, g::COL_BLACK);
    for(int i=0; i<slen; i++) fb::fill_rect(ax+4+sx[i]*10, ay+4+sy[i]*10, 10, 10, 0x00FF00);
    fb::fill_rect(ax+4+fx*10, ay+4+fy*10, 10, 10, 0xFF0000);
    if(s_dead) fb::draw_string(ax+aw/2-40, ay+ah/2, "GAME OVER", 0xFFFFFF, 0x000000);
  } else {
      g::draw_sunken(ax, ay, aw, ah); fb::fill_rect(ax+1, ay+1, aw-2, ah-2, g::COL_WHITE);
      fb::draw_string(ax+8, ay+10, "App logic coming soon...", g::COL_BLACK, g::COL_WHITE);
  }
}

void bring_to_front(int idx) {
    int pos=0; for(int i=0;i<NUM_WIN;i++) if(win_order[i]==idx) {pos=i;break;}
    for(int i=pos;i<NUM_WIN-1;i++) win_order[i]=win_order[i+1];
    win_order[NUM_WIN-1]=idx; mark_full();
}

void draw_scene(int mx, int my) {
  if (current_state == STATE_LOGIN) {
    fb::fill_rect(0, 0, 1280, 800, 0x000080);
    int lx=1280/2-150, ly=800/2-100;
    fb::fill_rect(lx, ly, 300, 200, g::COL_GRAY); g::draw_raised(lx, ly, 300, 200);
    fb::draw_string(lx+80, ly+15, "elevn32 Login", g::COL_BLACK, g::COL_GRAY);
    fb::draw_string(lx+20, ly+60, "User:", g::COL_BLACK, g::COL_GRAY);
    fb::fill_rect(lx+80, ly+55, 180, 24, g::COL_WHITE); g::draw_sunken(lx+80, ly+55, 180, 24);
    fb::draw_string(lx+84, ly+59, l_user, g::COL_BLACK, g::COL_WHITE);
    fb::draw_string(lx+20, ly+100, "Pass:", g::COL_BLACK, g::COL_GRAY);
    fb::fill_rect(lx+80, ly+95, 180, 24, g::COL_WHITE); g::draw_sunken(lx+80, ly+95, 180, 24);
    char stars[32]; for(int i=0;i<l_plen && i<31;i++) stars[i]='*'; stars[l_plen]=0;
    fb::draw_string(lx+84, ly+99, stars, g::COL_BLACK, g::COL_WHITE);
    draw_btn(lx+100, ly+140, 100, 30, "Login");
    if (l_focus==0) fb::fill_rect(lx+80, ly+78, 180, 2, 0x000080);
    if (l_focus==1) fb::fill_rect(lx+80, ly+118, 180, 2, 0x000080);
    return;
  }
  if (full_dirty) { fb::fill_rect(0, 0, 1280, 770, g::current_desktop_color); }
  else { for (int i=0; i<num_dirty; i++) { Rect &r = dirty_rects[i]; if (r.y < 770) { int32_t fh = (r.y+r.h>770)?770-r.y:r.h; fb::fill_rect(r.x, r.y, r.w, fh, g::current_desktop_color); } } }
  fb::fill_rect(20, 20, 32, 32, g::COL_BLACK); fb::draw_string(22, 22, ">_", g::COL_WHITE, g::COL_BLACK);
  fb::fill_rect(20, 90, 32, 32, 0x00FF00); fb::draw_string(22, 92, "S", g::COL_BLACK, 0x00FF00);
  for (int i=0; i<NUM_WIN; i++) { Window &w = windows[win_order[i]]; if (!w.active || w.minimized) continue; bool idirty = full_dirty; if (!idirty) { for (int d=0; d<num_dirty; d++) { Rect &r = dirty_rects[d]; if (!(w.x > r.x+r.w || w.x+w.w < r.x || w.y > r.y+r.h || w.y+w.h < r.y)) { idirty = true; break; } } } if (idirty) draw_window(w, win_order[i]); }
  bool tdirty = full_dirty || start_open; if (!tdirty) { for(int d=0; d<num_dirty; d++) if(dirty_rects[d].y+dirty_rects[d].h >= 770) { tdirty=true; break; } }
  if (tdirty) {
    int tby = 800-30; fb::fill_rect(0, tby, 1280, 30, g::COL_GRAY); fb::fill_rect(0, tby, 1280, 2, g::COL_WHITE); draw_btn(2, tby+3, 60, 24, "Start");
    int bx = 68; const char *nm[] = {"Term","Calc","Paint","Clock","Set","Note","Info","Snake","Mine"};
    for (int i=0; i<NUM_WIN; i++) if (windows[i].active) { fb::fill_rect(bx, tby+3, 54, 24, g::COL_GRAY); g::draw_sunken(bx, tby+3, 54, 24); fb::draw_string(bx+4, tby+7, nm[i], g::COL_BLACK, g::COL_GRAY); bx+=58; }
    uint64_t ts = timer::get_ticks()/1000; char tc[6]; tc[0]='0'+(ts/3600)%24/10; tc[1]='0'+(ts/3600)%24%10; tc[2]=':'; tc[3]='0'+(ts/60)%60/10; tc[4]='0'+(ts/60)%60%10; tc[5]=0;
    g::draw_sunken(1220, tby+3, 50, 24); fb::draw_string(1228, tby+7, tc, g::COL_BLACK, g::COL_GRAY);
    if (start_open) {
      int sx2=2, sy2=tby-195; fb::fill_rect(sx2, sy2, 150, 193, g::COL_GRAY); g::draw_raised(sx2, sy2, 150, 193);
      const char *si[] = {"Terminal", "Calculator", "Paint", "Clock", "Settings", "Notepad", "System Info", "Snake Game", "Logout"};
      for (int i=0; i<9; i++) { int iy=sy2+4+i*21; bool h = (mx>=sx2+2 && mx<=sx2+148 && my>=iy && my<=iy+19); if (h) fb::fill_rect(sx2+2, iy, 146, 19, 0x000080); fb::draw_string(sx2+8, iy+2, si[i], h?g::COL_WHITE:g::COL_BLACK, h?0x000080:g::COL_GRAY); }
    }
  }
}

void init() {
  fs::init(); for (int i=0; i<NUM_WIN; i++) { windows[i].active=false; windows[i].minimized=false; win_order[i]=i; }
  windows[0]=(Window){false,false,false,100,60,500,320,100,60,500,320,"Terminal"};
  windows[1]=(Window){false,false,false,350,100,240,310,350,100,240,310,"Calculator"};
  windows[2]=(Window){false,false,false,150,100,310,200,150,100,310,200,"Paint"};
  windows[3]=(Window){false,false,false,500,60,220,220,500,60,220,220,"Clock"};
  windows[4]=(Window){false,false,false,400,200,250,160,400,200,250,160,"Settings"};
  windows[5]=(Window){false,false,false,200,150,400,280,200,150,400,280,"Notepad"};
  windows[6]=(Window){false,false,false,80,80,320,180,80,80,320,180,"System Info"};
  windows[7]=(Window){false,false,false,450,150,250,250,450,150,250,250,"Snake"};
  windows[8]=(Window){false,false,false,100,100,300,300,100,100,300,300,"Minesweeper"};
  mark_full();
}

void run() {
  static bool prev_left = false; static int pmx=0, pmy=0;
  while (true) {
    char c = 0;
    if (keyboard::has_input()) c = keyboard::get_char();
    if (c) {
      if (current_state == STATE_LOGIN) {
        if (c=='\b') { if(l_focus==0 && l_ulen>0) l_user[--l_ulen]=0; if(l_focus==1 && l_plen>0) l_pass[--l_plen]=0; }
        else if (c=='\n') { if (strcmp(l_user,"root")==0 && strcmp(l_pass,"123")==0) { current_state=STATE_DESKTOP; mark_full(); } }
        else { if(l_focus==0 && l_ulen<31) { l_user[l_ulen++]=c; l_user[l_ulen]=0; } if(l_focus==1 && l_plen<31) { l_pass[l_plen++]=c; l_pass[l_plen]=0; } }
        mark_full();
      } else {
        int tw = win_order[NUM_WIN-1];
        if (tw == 0 && windows[0].active) {
            if (c=='\b') { if(sh_ptr>0) sh_in[--sh_ptr]=0; }
            else if (c=='\n') { if(sh_count<15) strcpy(sh_lines[sh_count++], sh_in); else { for(int i=0;i<14;i++) strcpy(sh_lines[i], sh_lines[i+1]); strcpy(sh_lines[14], sh_in); } sh_in[0]=0; sh_ptr=0; }
            else if (sh_ptr<63) { sh_in[sh_ptr++]=c; sh_in[sh_ptr]=0; }
            add_dirty(windows[0].x, windows[0].y, windows[0].w, windows[0].h);
        } else if (tw == 5 && windows[5].active) {
            if (c=='\b') { if(note_len>0) notepad[--note_len]=0; } else if (note_len<1023) { notepad[note_len++]=c; notepad[note_len]=0; }
            add_dirty(windows[5].x, windows[5].y, windows[5].w, windows[5].h);
        } else if (tw == 7 && windows[7].active) { if(c=='w') sdir=0; else if(c=='d') sdir=1; else if(c=='s') sdir=2; else if(c=='a') sdir=3; }
      }
    }
    int mx=mouse::get_x(), my=mouse::get_y(); bool ml=mouse::is_left_clicked(); bool clicked = (ml && !prev_left); uint64_t now_ms = timer::get_ticks();
    if (current_state == STATE_DESKTOP && windows[7].active && !s_dead && now_ms - last_s_move > 100) {
        last_s_move = now_ms; for(int i=slen-1; i>0; i--) { sx[i]=sx[i-1]; sy[i]=sy[i-1]; }
        if(sdir==0) sy[0]--; else if(sdir==1) sx[0]++; else if(sdir==2) sy[0]++; else if(sdir==3) sx[0]--;
        int mw=(windows[7].w-8)/10, mh=(windows[7].h-28-8)/10; if(sx[0]<0||sy[0]<0||sx[0]>=mw||sy[0]>=mh) s_dead=true;
        for(int i=1;i<slen;i++) if(sx[0]==sx[i]&&sy[0]==sy[i]) s_dead=true;
        if(sx[0]==fx&&sy[0]==fy){slen++;fx=(fx+7)%mw;fy=(fy+5)%mh;} add_dirty(windows[7].x, windows[7].y, windows[7].w, windows[7].h);
    }
    if (clicked) {
      if (current_state == STATE_LOGIN) {
        int lx=1280/2-150, ly=800/2-100;
        if (mx>=lx+80 && mx<=lx+260 && my>=ly+55 && my<=ly+79) l_focus=0;
        if (mx>=lx+80 && mx<=lx+260 && my>=ly+95 && my<=ly+119) l_focus=1;
        if (mx>=lx+100 && mx<=lx+200 && my>=ly+140 && my<=ly+170) { if(strcmp(l_user,"root")==0 && strcmp(l_pass,"123")==0) { current_state=STATE_DESKTOP; mark_full(); } }
        mark_full();
      } else {
        int tby=800-30;
        if (mx>=2 && mx<=62 && my>=tby+3 && my<=tby+27) { start_open=!start_open; mark_full(); }
        else if (start_open) {
          int sx2=2, sy2=tby-195;
          if (mx>=sx2 && mx<=sx2+150 && my>=sy2 && my<=sy2+193) {
            for (int i=0; i<9; i++) if (my>=sy2+4+i*21 && my<=sy2+4+i*21+19) { if(i==8) current_state=STATE_LOGIN; else { windows[i].active=true; bring_to_front(i); } start_open=false; mark_full(); break; }
          } else { start_open=false; mark_full(); }
        } else {
          int bx=68; bool tb_c=false;
          for(int i=0;i<NUM_WIN;i++) if(windows[i].active){if(mx>=bx&&mx<=bx+54&&my>=tby+3&&my<=tby+27){windows[i].minimized=false;bring_to_front(i);tb_c=true;break;} bx+=58;}
          if(!tb_c) {
            if(mx>=20&&mx<=80&&my>=20&&my<=80){windows[0].active=true;bring_to_front(0);}
            else if(mx>=20&&mx<=80&&my>=90&&my<=150){windows[7].active=true;bring_to_front(7);}
            else {
              for(int i=NUM_WIN-1;i>=0;i--){ int idx=win_order[i]; Window &w=windows[idx]; if(!w.active||w.minimized)continue;
                if(mx>=w.x&&mx<=w.x+w.w&&my>=w.y&&my<=w.y+w.h){ bring_to_front(idx);
                  if(mx>=w.x+7&&mx<=w.x+17&&my>=w.y+7&&my<=w.y+17) w.active=false;
                  else if(mx>=w.x+21&&mx<=w.x+31&&my>=w.y+7&&my<=w.y+17) w.minimized=true;
                  else if(mx>=w.x+35&&mx<=w.x+45&&my>=w.y+7&&my<=w.y+17){if(w.maximized){w.x=w.rx;w.y=w.ry;w.w=w.rw;w.h=w.rh;w.maximized=false;}else{w.rx=w.x;w.ry=w.y;w.rw=w.w;w.rh=w.h;w.x=0;w.y=0;w.w=1280;w.h=770;w.maximized=true;}}
                  else if(mx>=w.x+48&&mx<=w.x+w.w-3&&my>=w.y+3&&my<=w.y+21&&!w.maximized){drag_win=idx;drag_off_x=mx-w.x;drag_off_y=my-w.y;}
                  else if(idx==4){ int ax2=w.x+4, ay2=w.y+24; uint32_t ccs[]={0x008080,0x004040,0x800000,0x000080,0x2F4F2F}; for(int j=0;j<5;j++) if(mx>=ax2+10+j*38&&mx<=ax2+40+j*38&&my>=ay2+30&&my<=ay2+60){g::current_desktop_color=ccs[j];mark_full();} if(mx>=ax2+8&&mx<=ax2+88&&my>=ay2+88&&my<=ay2+112){g::COL_GRAY=0xC0C0C0;g::COL_WHITE=0xFFFFFF;g::COL_BLACK=0x000000;mark_full();} if(mx>=ax2+100&&mx<=ax2+180&&my>=ay2+88&&my<=ay2+112){g::COL_GRAY=0x2D2D30;g::COL_WHITE=0x1E1E1E;g::COL_BLACK=0xFFFFFF;mark_full();} }
                  else if(idx==5){ int ax2=w.x+4, ay2=w.y+24, aw2=w.w-8, ah2=w.h-28; if(mx>=ax2+2&&mx<=ax2+62&&my>=ay2+ah2-24&&my<=ay2+ah2-4) fs::save_file("note.txt",notepad,note_len); if(mx>=ax2+64&&mx<=ax2+124&&my>=ay2+ah2-24&&my<=ay2+ah2-4){int sz;fs::load_file("note.txt",notepad,&sz);note_len=sz;} }
                  mark_full(); break;
                }
              }
            }
          }
        }
      }
    }
    if (!ml && drag_win!=-1) { windows[drag_win].x=drag_target_x; windows[drag_win].y=drag_target_y; drag_win=-1; mark_full(); }
    if (drag_win!=-1) { int nx=mx-drag_off_x, ny=my-drag_off_y; if(nx!=drag_target_x||ny!=drag_target_y){add_dirty(drag_target_x,drag_target_y,windows[drag_win].w,windows[drag_win].h);drag_target_x=nx;drag_target_y=ny;add_dirty(nx,ny,windows[drag_win].w,windows[drag_win].h);} }
    prev_left=ml; uint64_t now=timer::get_ticks();
    if (scene_dirty && now-last_sc>=16) {
      draw_scene(mx, my); save_cur(mx, my); g::draw_cursor(mx, my);
      if (full_dirty) fb::swap_buffers(); else for (int i=0; i<num_dirty; i++) fb::swap_buffers_rect(dirty_rects[i].x, dirty_rects[i].y, dirty_rects[i].w, dirty_rects[i].h);
      reset_dirty(); scene_dirty=false; last_sc=now; pmx=mx; pmy=my;
    } else if (mx!=pmx || my!=pmy) { if (start_open) add_dirty(2, 800-30-195, 150, 193); restore_cur(); fb::swap_buffers_rect(csx, csy, 12, 18); save_cur(mx, my); g::draw_cursor(mx, my); fb::swap_buffers_rect(mx, my, 12, 18); pmx=mx; pmy=my; }
  }
}
}
