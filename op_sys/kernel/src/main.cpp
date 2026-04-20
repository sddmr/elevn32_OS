#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "framebuffer.h"
#include "string.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "heap.h"
#include "timer.h"
#include "keyboard.h"
#include "shell.h"
#include "serial.h"
#include "mouse.h"
#include "wm.h"
#include "boot_screen.h"

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = nullptr
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = nullptr
};

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

extern "C" {
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();
}

static void call_constructors() {
    for (void (**ctor)() = __init_array_start; ctor < __init_array_end; ctor++) {
        (*ctor)();
    }
}

static void halt() {
    for (;;) {
        asm volatile("hlt");
    }
}

static uint8_t kernel_heap[65536];

static uint64_t total_ram = 0;

extern "C" void kernel_start() {
    call_constructors();
    serial::init();
    serial::println("=== elevn32 kernel start ===");

    if (LIMINE_BASE_REVISION_SUPPORTED == false) halt();

    if (framebuffer_request.response == nullptr ||
        framebuffer_request.response->framebuffer_count < 1) halt();

    struct limine_framebuffer *lfb = framebuffer_request.response->framebuffers[0];

    fb::init(
        (uint32_t *)lfb->address,
        lfb->width,
        lfb->height,
        lfb->pitch
    );

    fb::clear(0x000000);

    fb::set_color(0x00FF88, 0x000000);
    fb::println("");
    fb::println("  elevn32 v0.3");
    fb::println("");

    fb::set_color(0x00DDFF, 0x000000);

    char buf[64];
    fb::print("  [INFO] ");
    fb::set_color(0xFFFFFF, 0x000000);
    fb::print("Display: ");
    fb::set_color(0xFFFFFF, 0x000000);
    uint_to_str(lfb->width, buf);
    fb::print(buf);
    fb::print("x");
    uint_to_str(lfb->height, buf);
    fb::print(buf);
    fb::print(" @ ");
    uint_to_str(lfb->bpp, buf);
    fb::print(buf);
    fb::println("bpp");

    gdt::init();
    fb::set_color(0x00FF88, 0x000000);
    fb::print("  [OK] ");
    fb::set_color(0xFFFFFF, 0x000000);
    fb::println("GDT initialized");

    isr::init();
    fb::set_color(0x00FF88, 0x000000);
    fb::print("  [OK] ");
    fb::set_color(0xFFFFFF, 0x000000);
    fb::println("IDT + PIC initialized");

    if (memmap_request.response) {
        for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) {
            struct limine_memmap_entry *e = memmap_request.response->entries[i];
            if (e->type == LIMINE_MEMMAP_USABLE) {
                total_ram += e->length;
            }
        }
    }

    fb::set_color(0x00FF88, 0x000000);
    fb::print("  [OK] ");
    fb::set_color(0xFFFFFF, 0x000000);
    fb::print("Detected RAM: ");
    uint_to_str(total_ram / 1024 / 1024, buf);
    fb::print(buf);
    fb::println(" MB");

    heap::init((uint64_t)kernel_heap, sizeof(kernel_heap));
    fb::set_color(0x00FF88, 0x000000);
    fb::print("  [OK] ");
    fb::set_color(0xFFFFFF, 0x000000);
    fb::println("Heap initialized");

    timer::init(100);
    fb::set_color(0x00FF88, 0x000000);
    fb::print("  [OK] ");
    fb::set_color(0xFFFFFF, 0x000000);
    fb::println("PIT timer");

    keyboard::init();
    fb::set_color(0x00FF88, 0x000000);
    fb::print("  [OK] ");
    fb::set_color(0xFFFFFF, 0x000000);
    fb::println("PS/2 keyboard");

    mouse::init();
    fb::set_color(0x00FF88, 0x000000);
    fb::print("  [OK] ");
    fb::set_color(0xFFFFFF, 0x000000);
    fb::println("PS/2 mouse");

    fb::println("");

    asm volatile("sti");
    
    // Slight pause to let the user see the boot sequence
    timer::sleep(1000);

    // Play boot animation
    boot::play_animation();

    wm::init();
    wm::run();

    halt();
}
