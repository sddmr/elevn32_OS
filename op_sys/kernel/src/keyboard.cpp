#include "keyboard.h"
#include "isr.h"
#include "io.h"
#include "framebuffer.h"
#include "serial.h"

namespace keyboard {

static const char scancode_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static const char scancode_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

#define BUFFER_SIZE 256

static char buffer[BUFFER_SIZE];
static volatile int buf_head = 0;
static volatile int buf_tail = 0;
static bool shift_pressed = false;

static void callback(InterruptFrame *) {
    uint8_t scancode = inb(0x60);

    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = true;
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = false;
        return;
    }

    if (scancode & 0x80) return;

    if (scancode < sizeof(scancode_ascii)) {
        char c = shift_pressed ? scancode_shift[scancode] : scancode_ascii[scancode];
        if (c) {
            int next = (buf_head + 1) % BUFFER_SIZE;
            if (next != buf_tail) {
                buffer[buf_head] = c;
                buf_head = next;
            }
        }
    }
}

void init() {
    buf_head = 0;
    buf_tail = 0;
    isr::register_handler(33, callback);

    uint8_t mask = inb(0x21);
    mask &= ~(1 << 1);
    outb(0x21, mask);
}

bool has_input() {
    return buf_head != buf_tail;
}

char get_char() {
    while (!has_input()) {
        asm volatile("hlt");
    }
    char c = buffer[buf_tail];
    buf_tail = (buf_tail + 1) % BUFFER_SIZE;
    return c;
}

void get_line(char *out, int max_len) {
    int pos = 0;
    while (pos < max_len - 1) {
        char c = get_char();

        if (c == '\n') {
            fb::print("\n");
            break;
        }

        if (c == '\b') {
            if (pos > 0) {
                pos--;
                fb::print("\b");
            }
            continue;
        }

        out[pos++] = c;
        char s[2] = {c, 0};
        fb::print(s);
    }
    out[pos] = '\0';
}

}
