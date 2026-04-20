#include "timer.h"
#include "isr.h"
#include "io.h"

namespace timer {

static uint64_t tick_count = 0;
static uint32_t freq = 0;

static void callback(InterruptFrame *) {
    tick_count = tick_count + 1;
}

void init(uint32_t frequency) {
    freq = frequency;
    tick_count = 0;
    isr::register_handler(32, callback);

    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));

    uint8_t mask = inb(0x21);
    mask &= ~(1 << 0);
    outb(0x21, mask);
}

uint64_t get_ticks() {
    return tick_count;
}

void sleep(uint64_t ms) {
    uint64_t target = tick_count + (ms * freq) / 1000;
    while (tick_count < target) {
        asm volatile("hlt");
    }
}

}
