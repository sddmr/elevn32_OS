#pragma once

#include <stdint.h>

struct InterruptFrame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

typedef uint64_t (*isr_handler_t)(InterruptFrame *frame);

namespace isr {
    void init();
    void register_handler(uint8_t n, isr_handler_t handler);
}
