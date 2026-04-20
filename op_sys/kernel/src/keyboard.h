#pragma once

#include <stdint.h>
#include "isr.h"

namespace keyboard {

void init();
char get_char();
bool has_input();
void get_line(char *buffer, int max_len);

}
