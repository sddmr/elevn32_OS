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
#include "vmm.h"
#include "sched.h"
#include "syscall.h"
#include "string.h"

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



extern "C" void halt() {
    for (;;) {
        asm volatile("hlt");
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

    uint64_t hhdm_offset = 0;
    if (hhdm_request.response) {
        hhdm_offset = hhdm_request.response->offset;
    }

    if (memmap_request.response) {
        pmm::init(memmap_request.response, hhdm_offset);
        fb::set_color(0x00FF88, 0x000000);
        fb::print("  [OK] ");
        fb::set_color(0xFFFFFF, 0x000000);
        fb::println("PMM initialized");
    }

    vmm::init(hhdm_offset);
    fb::set_color(0x00FF88, 0x000000);
    fb::print("  [OK] ");
    fb::set_color(0xFFFFFF, 0x000000);
    fb::println("VMM (Paging) initialized");

    heap::init();
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

    sched::init();
    fb::set_color(0x00FF88, 0x000000);
    fb::print("  [OK] ");
    fb::set_color(0xFFFFFF, 0x000000);
    fb::println("Scheduler initialized");

    syscall::init();
    fb::set_color(0x00FF88, 0x000000);
    fb::print("  [OK] ");
    fb::set_color(0xFFFFFF, 0x000000);
    fb::println("Syscall (Ring 3) ready");

    asm volatile("sti");
    
    // User mode test code
    uint8_t *user_code_page = (uint8_t *)pmm::alloc_page();
    // Map it to 0x400000 (User Space)
    vmm::map_page(vmm::get_kernel_pml4(), 0x400000, (uint64_t)user_code_page - hhdm_offset, vmm::PTE_PRESENT | vmm::PTE_WRITABLE | vmm::PTE_USER);
    
    // x86_64 machine code for:
    // mov rax, 1
    // mov rdi, 0x400020
    // syscall
    // mov rax, 0
    // syscall
    // string: "Hello from Ring 3!\n\0"
    
    uint8_t payload[] = {
        0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, // mov rax, 1
        0x48, 0xC7, 0xC7, 0x20, 0x00, 0x40, 0x00, // mov rdi, 0x400020
        0x0F, 0x05,                               // syscall
        0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00, // mov rax, 0
        0x0F, 0x05                                // syscall
    };
    for (size_t i = 0; i < sizeof(payload); i++) user_code_page[i] = payload[i];
    
    const char *msg = "Hello from Ring 3 (User Mode)!\n";
    for (size_t i = 0; msg[i]; i++) user_code_page[0x20 + i] = msg[i];
    user_code_page[0x20 + strlen(msg)] = 0;
    
    uint8_t *user_stack_page = (uint8_t *)pmm::alloc_page();
    vmm::map_page(vmm::get_kernel_pml4(), 0x800000, (uint64_t)user_stack_page - hhdm_offset, vmm::PTE_PRESENT | vmm::PTE_WRITABLE | vmm::PTE_USER | vmm::PTE_NX);

    sched::create_kernel_thread([]() {
        while (true) {
            timer::sleep(500);
            fb::swap_buffers_rect(1200, 10, 10, 10);
        }
    });
    
    // Create a thread that switches to user mode!
    sched::create_kernel_thread([]() {
        asm volatile(
            "cli\n"
            "movw $27, %%ax\n"
            "movw %%ax, %%ds\n"
            "movw %%ax, %%es\n"
            "movw %%ax, %%fs\n"
            "movw %%ax, %%gs\n"
            "pushq $27\n"
            "pushq $0x801000\n"
            "pushq $0x202\n"
            "pushq $35\n"
            "pushq $0x400000\n"
            "iretq\n"
            : : : "memory"
        );
    });

    timer::sleep(1000);

    boot::play_animation();

    wm::init();
    wm::run();

    halt();
}
