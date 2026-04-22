#include <stdint.h>
#include <stddef.h>
#include <limine.h>
#include "framebuffer.h"
#include "graphics.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "pmm.h"
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

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

static uint8_t kernel_heap[1024 * 1024 * 4]; // 4MB heap

extern "C" void halt() {
    for (;;) {
        asm volatile("hlt");
    }
}

void uint_to_str(uint64_t n, char* str) {
    if (n == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    int i = 0;
    while (n > 0) {
        str[i++] = (n % 10) + '0';
        n /= 10;
    }
    str[i] = '\0';
    for (int j = 0; j < i / 2; j++) {
        char tmp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = tmp;
    }
}

extern "C" void kernel_start(void) {
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        halt();
    }

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    fb::init((uint32_t*)fb->address, fb->width, fb->height, fb->pitch);
    fb::clear(0x000000);

    serial::init();
    serial::print("Elevn32 OS Kernel Starting...\n");

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

    uint64_t total_ram = 0;
    char buf[32];
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
    timer::sleep(1000);

    boot::play_animation();

    wm::init();
    wm::run();

    halt();
}
