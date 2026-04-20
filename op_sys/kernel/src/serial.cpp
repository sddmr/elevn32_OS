#include "serial.h"
#include "io.h"

namespace serial {

#define PORT 0x3f8 // COM1

void init() {
    outb(PORT + 1, 0x00);
    outb(PORT + 3, 0x80);
    outb(PORT + 0, 0x03);
    outb(PORT + 1, 0x00);
    outb(PORT + 3, 0x03);
    outb(PORT + 2, 0xC7);
    outb(PORT + 4, 0x0B);
}

static int is_transmit_empty() {
    return inb(PORT + 5) & 0x20;
}

void print(const char *str) {
    while (*str) {
        while (is_transmit_empty() == 0);
        outb(PORT, *str);
        str++;
    }
}

void println(const char *str) {
    print(str);
    print("\n");
}

void print_hex(uint8_t val) {
    const char *hex = "0123456789ABCDEF";
    char buf[3] = {hex[(val >> 4) & 0xF], hex[val & 0xF], 0};
    print(buf);
}

}
