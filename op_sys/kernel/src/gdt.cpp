#include "gdt.h"

namespace gdt {

static GDTEntry entries[5];
static GDTPointer pointer;

static void set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    entries[index].base_low = base & 0xFFFF;
    entries[index].base_middle = (base >> 16) & 0xFF;
    entries[index].base_high = (base >> 24) & 0xFF;
    entries[index].limit_low = limit & 0xFFFF;
    entries[index].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    entries[index].access = access;
}

extern "C" void gdt_flush(uint64_t gdt_ptr);

void init() {
    pointer.limit = sizeof(entries) - 1;
    pointer.base = (uint64_t)&entries;

    set_entry(0, 0, 0, 0, 0);
    set_entry(1, 0, 0xFFFFF, 0x9A, 0xA0);
    set_entry(2, 0, 0xFFFFF, 0x92, 0xC0);
    set_entry(3, 0, 0xFFFFF, 0xF2, 0xC0); // User Data (0x18)
    set_entry(4, 0, 0xFFFFF, 0xFA, 0xA0); // User Code (0x20)

    gdt_flush((uint64_t)&pointer);
}

}
