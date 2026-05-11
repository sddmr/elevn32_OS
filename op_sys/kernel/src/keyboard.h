#pragma once

#include <stdint.h>
#include "isr.h"

namespace keyboard {

enum SpecialKey {
    KEY_NONE = 0,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_DELETE,
    KEY_ESCAPE
};

void init();
char get_char();
bool has_input();
SpecialKey get_special_key();
bool ctrl_down();
bool alt_down();
void get_line(char *buffer, int max_len);

}
