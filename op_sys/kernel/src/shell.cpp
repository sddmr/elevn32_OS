#include "shell.h"
#include "framebuffer.h"
#include "fs.h"
#include "keyboard.h"
#include "string.h"

#include "heap.h"
#include "io.h"
#include "timer.h"

namespace shell {

static void cmd_help() {
  fb::set_color(0x00DDFF, 0x000000);
  fb::println("  Available commands:");
  fb::set_color(0xFFFFFF, 0x000000);
  fb::println("    help     - Show this message");
  fb::println("    clear    - Clear screen");
  fb::println("    info     - System information");
  fb::println("    mem      - Memory statistics");
  fb::println("    ls       - List current directory");
  fb::println("    pwd      - Print current directory");
  fb::println("    cd       - Change directory");
  fb::println("    mkdir    - Create directory");
  fb::println("    touch    - Create empty file");
  fb::println("    cat      - Print file contents");
  fb::println("    echo     - Print text");
  fb::println("    color    - Change text color");
  fb::println("    uptime   - Show system uptime");
  fb::println("    reboot   - Reboot system");
}

static void cmd_info() {
  fb::set_color(0x00FF88, 0x000000);
  fb::println("  elevn32 v0.3 - System Info");
  fb::set_color(0xFFFFFF, 0x000000);
  fb::println("  Arch:     x86_64");
  fb::println("  Language: C++20");
  fb::println("  Loader:   Limine v8");
  fb::println("  Features: GDT IDT PIC PMM Heap KB Timer Shell");
}

static void cmd_mem() {
  char buf[32];
  fb::set_color(0x00DDFF, 0x000000);
  fb::println("  Memory Statistics:");
  fb::set_color(0xFFFFFF, 0x000000);

  fb::print("    Heap Used: ");
  uint_to_str(heap::get_used(), buf);
  fb::print(buf);
  fb::println(" bytes");

  fb::print("    Heap Free: ");
  uint_to_str(heap::get_free(), buf);
  fb::print(buf);
  fb::println(" bytes");
}

static void cmd_uptime() {
  char buf[32];
  uint64_t ticks = timer::get_ticks();
  uint64_t seconds = ticks / 100;
  uint64_t minutes = seconds / 60;

  fb::set_color(0xFFFFFF, 0x000000);
  fb::print("  Uptime: ");
  uint_to_str(minutes, buf);
  fb::print(buf);
  fb::print("m ");
  uint_to_str(seconds % 60, buf);
  fb::print(buf);
  fb::print("s (");
  uint_to_str(ticks, buf);
  fb::print(buf);
  fb::println(" ticks)");
}

static void cmd_echo(const char *args) {
  fb::set_color(0xFFFFFF, 0x000000);
  fb::print("  ");
  fb::println(args);
}

static void cmd_ls() {
  int count = fs::get_file_count();
  fb::set_color(0xFFFFFF, 0x000000);
  if (count == 0) {
    fb::println("  <empty>");
    return;
  }
  for (int i = 0; i < count; i++) {
    fb::print("  ");
    fb::println(fs::get_file_name(i));
  }
}

static void cmd_pwd() {
  fb::set_color(0xFFFFFF, 0x000000);
  fb::print("  ");
  fb::println(fs::getcwd());
}

static void cmd_cd(const char *args) {
  const char *target = strlen(args) == 0 ? "/" : args;
  if (!fs::chdir(target)) {
    fb::set_color(0xFF4444, 0x000000);
    fb::print("  [NO] ");
    fb::set_color(0xFFFFFF, 0x000000);
    fb::println("Directory not found");
  }
}

static void cmd_mkdir(const char *args) {
  if (strlen(args) == 0 || !fs::mkdir(args)) {
    fb::set_color(0xFF4444, 0x000000);
    fb::print("  [NO] ");
    fb::set_color(0xFFFFFF, 0x000000);
    fb::println("Could not create directory");
    return;
  }
  fb::set_color(0x00FF88, 0x000000);
  fb::println("  Directory created");
}

static void cmd_touch(const char *args) {
  if (strlen(args) == 0 || !fs::touch(args)) {
    fb::set_color(0xFF4444, 0x000000);
    fb::print("  [NO] ");
    fb::set_color(0xFFFFFF, 0x000000);
    fb::println("Could not create file");
    return;
  }
  fb::set_color(0x00FF88, 0x000000);
  fb::println("  File ready");
}

static void cmd_cat(const char *args) {
  char content[1025];
  int size = 0;
  if (strlen(args) == 0 || !fs::load_file(args, content, &size)) {
    fb::set_color(0xFF4444, 0x000000);
    fb::print("  [NO] ");
    fb::set_color(0xFFFFFF, 0x000000);
    fb::println("File not found");
    return;
  }
  fb::set_color(0xFFFFFF, 0x000000);
  fb::print("  ");
  fb::println(content);
}

static void cmd_color(const char *args) {
  uint32_t color = 0xFFFFFF;
  if (strcmp(args, "red") == 0)
    color = 0xFF4444;
  else if (strcmp(args, "green") == 0)
    color = 0x00FF88;
  else if (strcmp(args, "blue") == 0)
    color = 0x4488FF;
  else if (strcmp(args, "yellow") == 0)
    color = 0xFFDD00;
  else if (strcmp(args, "cyan") == 0)
    color = 0x00DDFF;
  else if (strcmp(args, "white") == 0)
    color = 0xFFFFFF;
  else if (strcmp(args, "pink") == 0)
    color = 0xFF66AA;
  else if (strcmp(args, "orange") == 0)
    color = 0xFF8844;
  else {
    fb::set_color(0xFF4444, 0x000000);
    fb::print("  [NO] ");
    fb::set_color(0xFFFFFF, 0x000000);
    fb::println("Invalid color. Available: red green blue yellow cyan white "
                "pink orange");
    return;
  }
  fb::set_color(color, 0x000000);
  fb::println("  Color changed.");
}

static void cmd_reboot() {
  fb::set_color(0xFFDD00, 0x000000);
  fb::println("  Rebooting...");
  uint8_t good = 0x02;
  while (good & 0x02) {
    good = inb(0x64);
  }
  outb(0x64, 0xFE);
  for (;;)
    asm volatile("hlt");
}

static void process_command(char *input) {
  if (strlen(input) == 0)
    return;

  char cmd[64] = {0};
  char args[192] = {0};

  int i = 0;
  while (input[i] && input[i] != ' ' && i < 63) {
    cmd[i] = input[i];
    i++;
  }
  cmd[i] = '\0';

  if (input[i] == ' ') {
    i++;
    int j = 0;
    while (input[i] && j < 191) {
      args[j++] = input[i++];
    }
    args[j] = '\0';
  }

  if (strcmp(cmd, "help") == 0)
    cmd_help();
  else if (strcmp(cmd, "clear") == 0)
    fb::clear(0x000000);
  else if (strcmp(cmd, "info") == 0)
    cmd_info();
  else if (strcmp(cmd, "mem") == 0)
    cmd_mem();
  else if (strcmp(cmd, "ls") == 0)
    cmd_ls();
  else if (strcmp(cmd, "pwd") == 0)
    cmd_pwd();
  else if (strcmp(cmd, "cd") == 0)
    cmd_cd(args);
  else if (strcmp(cmd, "mkdir") == 0)
    cmd_mkdir(args);
  else if (strcmp(cmd, "touch") == 0)
    cmd_touch(args);
  else if (strcmp(cmd, "cat") == 0)
    cmd_cat(args);
  else if (strcmp(cmd, "uptime") == 0)
    cmd_uptime();
  else if (strcmp(cmd, "echo") == 0)
    cmd_echo(args);
  else if (strcmp(cmd, "color") == 0)
    cmd_color(args);
  else if (strcmp(cmd, "reboot") == 0)
    cmd_reboot();
  else {
    fb::set_color(0xFF4444, 0x000000);
    fb::print("  [NO] ");
    fb::set_color(0xFFFFFF, 0x000000);
    fb::print("Unknown command: ");
    fb::println(cmd);
  }
}

void init() {
  fb::set_color(0x00FF88, 0x000000);
  fb::print("  [SHELL] ");
  fb::set_color(0xFFFFFF, 0x000000);
  fb::println("elevn32 shell initialized");
  fb::println("  Type 'help' for available commands.");
  fb::println("");
}

void run() {
  char input[256];

  while (true) {
    fb::set_color(0x00FF88, 0x000000);
    fb::print("  elevn32> ");
    fb::set_color(0xFFFFFF, 0x000000);

    keyboard::get_line(input, 256);
    process_command(input);
  }
}

// ── WM Terminal entegrasyonu
// ────────────────────────────────────────────────── fb:: kullanmaz; çıktıyı
// buf[][64] dizisine yazar, *count'u artırır.
static void wm_push(char buf[][64], int *count, const char *line) {
  if (*count >= 18) {
    for (int i = 0; i < 17; i++)
      strcpy(buf[i], buf[i + 1]);
    *count = 17;
  }
  int j = 0;
  while (line[j] && j < 63) {
    buf[*count][j] = line[j];
    j++;
  }
  buf[*count][j] = 0;
  (*count)++;
}

void process_command_wm(const char *input, char buf[][64], int *count) {
  if (strlen(input) == 0)
    return;

  char cmd[64] = {0};
  char args[192] = {0};
  int i = 0;
  while (input[i] && input[i] != ' ' && i < 63) {
    cmd[i] = input[i];
    i++;
  }
  cmd[i] = '\0';
  if (input[i] == ' ') {
    i++;
    int j = 0;
    while (input[i] && j < 191)
      args[j++] = input[i++];
    args[j] = 0;
  }

  if (strcmp(cmd, "help") == 0) {
    wm_push(buf, count, "Commands: help clear info mem uptime echo reboot");
    wm_push(buf, count, "          ls pwd cd mkdir touch cat");
  } else if (strcmp(cmd, "clear") == 0) {
    *count = 0;
  } else if (strcmp(cmd, "info") == 0) {
    wm_push(buf, count, "elevn32 v1.1.1 | x86_64 | Limine v8 | C++20");
  } else if (strcmp(cmd, "mem") == 0) {
    char line[64] = "Heap used: ";
    char num[16];
    uint_to_str(heap::get_used() / 1024, num);
    strcat(line, num);
    strcat(line, " KB  free: ");
    uint_to_str(heap::get_free() / 1024, num);
    strcat(line, num);
    strcat(line, " KB");
    wm_push(buf, count, line);
  } else if (strcmp(cmd, "uptime") == 0) {
    char line[64] = "Uptime: ";
    char num[16];
    uint64_t s = timer::get_ticks() / 100;
    uint_to_str(s / 60, num);
    strcat(line, num);
    strcat(line, "m ");
    uint_to_str(s % 60, num);
    strcat(line, num);
    strcat(line, "s");
    wm_push(buf, count, line);
  } else if (strcmp(cmd, "echo") == 0) {
    wm_push(buf, count, args);
  } else if (strcmp(cmd, "ls") == 0) {
    int fcount = fs::get_file_count();
    if (fcount == 0) {
      wm_push(buf, count, "<empty>");
    } else {
      for (int j = 0; j < fcount; j++) {
        const char *name = fs::get_file_name(j);
        if (name) wm_push(buf, count, name);
      }
    }
  } else if (strcmp(cmd, "pwd") == 0) {
    wm_push(buf, count, fs::getcwd());
  } else if (strcmp(cmd, "cd") == 0) {
    const char *target = strlen(args) == 0 ? "/" : args;
    if (!fs::chdir(target))
      wm_push(buf, count, "cd: directory not found");
  } else if (strcmp(cmd, "mkdir") == 0) {
    if (strlen(args) == 0 || !fs::mkdir(args))
      wm_push(buf, count, "mkdir: could not create directory");
    else
      wm_push(buf, count, "directory created");
  } else if (strcmp(cmd, "touch") == 0) {
    if (strlen(args) == 0 || !fs::touch(args))
      wm_push(buf, count, "touch: could not create file");
    else
      wm_push(buf, count, "file ready");
  } else if (strcmp(cmd, "cat") == 0) {
    char content[1025];
    int size = 0;
    if (strlen(args) == 0 || !fs::load_file(args, content, &size))
      wm_push(buf, count, "cat: file not found");
    else
      wm_push(buf, count, content);
  } else if (strcmp(cmd, "reboot") == 0) {
    wm_push(buf, count, "Rebooting...");
    uint8_t good = 0x02;
    while (good & 0x02)
      good = inb(0x64);
    outb(0x64, 0xFE);
    for (;;)
      asm volatile("hlt");
  } else {
    char line[64] = "Unknown: ";
    strcat(line, cmd);
    wm_push(buf, count, line);
  }
}

} // namespace shell
