#pragma once

#include <stdint.h>
#include <stddef.h>

namespace pmm {

void init(void *memmap_response, uint64_t hhdm_offset);
void *alloc_page();
void free_page(void *addr);
uint64_t get_total_memory();
uint64_t get_used_memory();
uint64_t get_free_memory();

}
