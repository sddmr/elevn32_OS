#include "idt.h"
#include "string.h"

namespace idt {

static IDTEntry entries[256];
static IDTPointer pointer;

void set_gate(uint8_t num, uint64_t handler, uint16_t selector, uint8_t flags) {
    entries[num].offset_low = handler & 0xFFFF;
    entries[num].offset_mid = (handler >> 16) & 0xFFFF;
    entries[num].offset_high = (handler >> 32) & 0xFFFFFFFF;
    entries[num].selector = selector;
    entries[num].ist = 0;
    entries[num].type_attr = flags;
    entries[num].zero = 0;
}

void init() {
    pointer.limit = sizeof(entries) - 1;
    pointer.base = (uint64_t)&entries;
    asm volatile("lidt %0" : : "m"(pointer));
}

}
