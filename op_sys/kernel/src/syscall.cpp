#include "syscall.h"
#include "cpu.h"
#include "framebuffer.h"
#include "string.h"
#include "sched.h"

extern "C" void syscall_entry();

namespace syscall {

#define IA32_EFER 0xC0000080
#define IA32_STAR 0xC0000081
#define IA32_LSTAR 0xC0000082
#define IA32_FMASK 0xC0000084

void init() {
    // Enable SCE (System Call Extensions) in EFER
    uint64_t efer = cpu::rdmsr(IA32_EFER);
    efer |= 1; // SCE bit is bit 0
    cpu::wrmsr(IA32_EFER, efer);
    
    // Set up STAR
    // Bits 47:32 -> Kernel CS (0x08) and SS (0x10)
    // Bits 63:48 -> User CS (0x1B) and SS (0x23)
    uint64_t star = ((uint64_t)0x08 << 32) | ((uint64_t)0x10 << 48);
    cpu::wrmsr(IA32_STAR, star);
    
    // Set LSTAR to our assembly entry point
    cpu::wrmsr(IA32_LSTAR, (uint64_t)syscall_entry);
    
    // Mask interrupts when entering syscall
    cpu::wrmsr(IA32_FMASK, 0x202);
}

}

extern "C" uint64_t syscall_handler(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    uint64_t syscall_number;
    asm volatile("mov %%rax, %0" : "=r"(syscall_number));
    
    if (syscall_number == 1) { // sys_print
        const char *str = (const char *)arg0;
        fb::print(str);
        return 0;
    } else if (syscall_number == 0) { // sys_exit
        fb::println("User program exited.");
        sched::exit_current_thread();
        return 0;
    }
    
    return -1;
}
