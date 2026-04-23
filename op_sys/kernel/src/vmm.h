#pragma once

#include <stdint.h>
#include <stddef.h>

namespace vmm {

// Page Table Entry flags
constexpr uint64_t PTE_PRESENT = 1ull << 0;
constexpr uint64_t PTE_WRITABLE = 1ull << 1;
constexpr uint64_t PTE_USER = 1ull << 2;
constexpr uint64_t PTE_NX = 1ull << 63; // No-execute

// Address mask to extract physical address from PTE
constexpr uint64_t PTE_ADDR_MASK = 0x000ffffffffff000ull;

typedef uint64_t* PageTable;

void init(uint64_t hhdm_offset);

// Map a physical page to a virtual page in the given PML4
void map_page(PageTable pml4, uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags);

// Switch the current page directory (CR3)
void switch_pml4(PageTable pml4);

// Get the kernel's main page directory
PageTable get_kernel_pml4();

}
