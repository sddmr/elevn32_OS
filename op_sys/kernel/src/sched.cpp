#include "sched.h"
#include "heap.h"

namespace sched {

constexpr int MAX_THREADS = 64;
static Thread threads[MAX_THREADS];
static int current_thread_idx = -1;
static int num_threads = 0;
static bool scheduler_enabled = false;

void init() {
    for (int i = 0; i < MAX_THREADS; i++) {
        threads[i].active = false;
    }
    
    threads[0].id = 0;
    threads[0].active = true;
    
    current_thread_idx = 0;
    num_threads = 1;
    scheduler_enabled = true;
}

uint64_t schedule(uint64_t current_rsp) {
    if (!scheduler_enabled || num_threads <= 1) return current_rsp;
    
    // Save current thread's state
    threads[current_thread_idx].rsp = current_rsp;
    
    // Round-robin
    do {
        current_thread_idx = (current_thread_idx + 1) % MAX_THREADS;
    } while (!threads[current_thread_idx].active);
    
    return threads[current_thread_idx].rsp;
}

void exit_current_thread() {
    if (!scheduler_enabled || num_threads <= 1) {
        for(;;) asm volatile("hlt");
    }
    
    threads[current_thread_idx].active = false;
    num_threads--;
    
    // Trigger a context switch immediately
    asm volatile("int $32");
    
    for(;;) asm volatile("hlt");
}

Thread* create_kernel_thread(void (*entry_point)()) {
    if (num_threads >= MAX_THREADS) return nullptr;
    
    int free_idx = -1;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (!threads[i].active) {
            free_idx = i;
            break;
        }
    }
    
    if (free_idx == -1) return nullptr;
    
    Thread *t = &threads[free_idx];
    t->id = free_idx;
    t->stack_size = 4096 * 4; // 16KB stack
    t->stack_base = new uint8_t[t->stack_size];
    
    uint64_t *stack = (uint64_t *)((uint8_t *)t->stack_base + t->stack_size);
    
    *--stack = 0x10; // SS
    uint64_t fake_rsp = (uint64_t)stack;
    *--stack = fake_rsp; // RSP
    *--stack = 0x202; // RFLAGS
    *--stack = 0x08; // CS
    *--stack = (uint64_t)entry_point; // RIP
    
    *--stack = 0; // err_code
    *--stack = 0; // int_no
    
    *--stack = 0; // rax
    *--stack = 0; // rbx
    *--stack = 0; // rcx
    *--stack = 0; // rdx
    *--stack = 0; // rsi
    *--stack = 0; // rdi
    *--stack = 0; // rbp
    *--stack = 0; // r8
    *--stack = 0; // r9
    *--stack = 0; // r10
    *--stack = 0; // r11
    *--stack = 0; // r12
    *--stack = 0; // r13
    *--stack = 0; // r14
    *--stack = 0; // r15
    
    t->rsp = (uint64_t)stack;
    t->active = true;
    num_threads++;
    
    return t;
}

}
