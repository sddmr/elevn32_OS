#include "isr.h"
#include "idt.h"
#include "io.h"
#include "framebuffer.h"
#include "string.h"

namespace isr {

static isr_handler_t handlers[256] = {0};

static const char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point",
    "Virtualization",
    "Control Protection",
};

extern "C" uint64_t isr_handler(InterruptFrame *frame) {
    if (frame->int_no < 32) {
        fb::set_color(0xFF4444, 0x000000);
        fb::println("");
        fb::print("  [EXCEPTION] ");
        if (frame->int_no < 22) {
            fb::println(exception_messages[frame->int_no]);
        } else {
            fb::println("Unknown Exception");
        }
        char buf[32];
        fb::print("  [INT] Number: ");
        uint_to_str(frame->int_no, buf);
        fb::println(buf);
        fb::print("  [ERR] Code: 0x");
        uint_to_str(frame->err_code, buf, 16);
        fb::println(buf);
        for(;;) asm volatile("hlt");
    }

    uint64_t new_rsp = (uint64_t)frame;

    if (handlers[frame->int_no]) {
        new_rsp = handlers[frame->int_no](frame);
    }

    if (frame->int_no >= 32 && frame->int_no < 48) {
        if (frame->int_no >= 40) {
            outb(0xA0, 0x20);
        }
        outb(0x20, 0x20);
    }
    
    return new_rsp;
}

void register_handler(uint8_t n, isr_handler_t handler) {
    handlers[n] = handler;
}

extern "C" void isr0(); extern "C" void isr1(); extern "C" void isr2();
extern "C" void isr3(); extern "C" void isr4(); extern "C" void isr5();
extern "C" void isr6(); extern "C" void isr7(); extern "C" void isr8();
extern "C" void isr9(); extern "C" void isr10(); extern "C" void isr11();
extern "C" void isr12(); extern "C" void isr13(); extern "C" void isr14();
extern "C" void isr15(); extern "C" void isr16(); extern "C" void isr17();
extern "C" void isr18(); extern "C" void isr19(); extern "C" void isr20();
extern "C" void isr21(); extern "C" void isr22(); extern "C" void isr23();
extern "C" void isr24(); extern "C" void isr25(); extern "C" void isr26();
extern "C" void isr27(); extern "C" void isr28(); extern "C" void isr29();
extern "C" void isr30(); extern "C" void isr31();

extern "C" void irq0(); extern "C" void irq1(); extern "C" void irq2();
extern "C" void irq3(); extern "C" void irq4(); extern "C" void irq5();
extern "C" void irq6(); extern "C" void irq7(); extern "C" void irq8();
extern "C" void irq9(); extern "C" void irq10(); extern "C" void irq11();
extern "C" void irq12(); extern "C" void irq13(); extern "C" void irq14();
extern "C" void irq15();

static void remap_pic() {
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();
    outb(0xA1, 0x28); io_wait();
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();
    outb(0x21, 0xFF); io_wait();
    outb(0xA1, 0xFF); io_wait();
}

void init() {
    remap_pic();

    idt::set_gate(0, (uint64_t)isr0, 0x08, 0x8E);
    idt::set_gate(1, (uint64_t)isr1, 0x08, 0x8E);
    idt::set_gate(2, (uint64_t)isr2, 0x08, 0x8E);
    idt::set_gate(3, (uint64_t)isr3, 0x08, 0x8E);
    idt::set_gate(4, (uint64_t)isr4, 0x08, 0x8E);
    idt::set_gate(5, (uint64_t)isr5, 0x08, 0x8E);
    idt::set_gate(6, (uint64_t)isr6, 0x08, 0x8E);
    idt::set_gate(7, (uint64_t)isr7, 0x08, 0x8E);
    idt::set_gate(8, (uint64_t)isr8, 0x08, 0x8E);
    idt::set_gate(9, (uint64_t)isr9, 0x08, 0x8E);
    idt::set_gate(10, (uint64_t)isr10, 0x08, 0x8E);
    idt::set_gate(11, (uint64_t)isr11, 0x08, 0x8E);
    idt::set_gate(12, (uint64_t)isr12, 0x08, 0x8E);
    idt::set_gate(13, (uint64_t)isr13, 0x08, 0x8E);
    idt::set_gate(14, (uint64_t)isr14, 0x08, 0x8E);
    idt::set_gate(15, (uint64_t)isr15, 0x08, 0x8E);
    idt::set_gate(16, (uint64_t)isr16, 0x08, 0x8E);
    idt::set_gate(17, (uint64_t)isr17, 0x08, 0x8E);
    idt::set_gate(18, (uint64_t)isr18, 0x08, 0x8E);
    idt::set_gate(19, (uint64_t)isr19, 0x08, 0x8E);
    idt::set_gate(20, (uint64_t)isr20, 0x08, 0x8E);
    idt::set_gate(21, (uint64_t)isr21, 0x08, 0x8E);
    idt::set_gate(22, (uint64_t)isr22, 0x08, 0x8E);
    idt::set_gate(23, (uint64_t)isr23, 0x08, 0x8E);
    idt::set_gate(24, (uint64_t)isr24, 0x08, 0x8E);
    idt::set_gate(25, (uint64_t)isr25, 0x08, 0x8E);
    idt::set_gate(26, (uint64_t)isr26, 0x08, 0x8E);
    idt::set_gate(27, (uint64_t)isr27, 0x08, 0x8E);
    idt::set_gate(28, (uint64_t)isr28, 0x08, 0x8E);
    idt::set_gate(29, (uint64_t)isr29, 0x08, 0x8E);
    idt::set_gate(30, (uint64_t)isr30, 0x08, 0x8E);
    idt::set_gate(31, (uint64_t)isr31, 0x08, 0x8E);

    idt::set_gate(32, (uint64_t)irq0, 0x08, 0x8E);
    idt::set_gate(33, (uint64_t)irq1, 0x08, 0x8E);
    idt::set_gate(34, (uint64_t)irq2, 0x08, 0x8E);
    idt::set_gate(35, (uint64_t)irq3, 0x08, 0x8E);
    idt::set_gate(36, (uint64_t)irq4, 0x08, 0x8E);
    idt::set_gate(37, (uint64_t)irq5, 0x08, 0x8E);
    idt::set_gate(38, (uint64_t)irq6, 0x08, 0x8E);
    idt::set_gate(39, (uint64_t)irq7, 0x08, 0x8E);
    idt::set_gate(40, (uint64_t)irq8, 0x08, 0x8E);
    idt::set_gate(41, (uint64_t)irq9, 0x08, 0x8E);
    idt::set_gate(42, (uint64_t)irq10, 0x08, 0x8E);
    idt::set_gate(43, (uint64_t)irq11, 0x08, 0x8E);
    idt::set_gate(44, (uint64_t)irq12, 0x08, 0x8E);
    idt::set_gate(45, (uint64_t)irq13, 0x08, 0x8E);
    idt::set_gate(46, (uint64_t)irq14, 0x08, 0x8E);
    idt::set_gate(47, (uint64_t)irq15, 0x08, 0x8E);

    idt::init();
}

}
