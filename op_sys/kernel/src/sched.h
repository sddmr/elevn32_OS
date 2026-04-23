#pragma once

#include <stdint.h>
#include <stddef.h>
#include "isr.h"

namespace sched {

struct Thread {
    uint64_t id;
    uint64_t rsp;
    uint64_t cr3;
    bool active;
    void *stack_base;
    uint64_t stack_size;
};

void init();
uint64_t schedule(uint64_t current_rsp);
Thread* create_kernel_thread(void (*entry_point)());
void exit_current_thread();

}
