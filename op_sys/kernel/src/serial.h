#pragma once
#include <stdint.h>

namespace serial {
    void init();
    void print(const char *str);
    void println(const char *str);
    void print_hex(uint8_t val);
}
