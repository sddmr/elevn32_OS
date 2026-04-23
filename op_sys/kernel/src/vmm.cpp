#include "vmm.h"
#include "pmm.h"
#include "string.h"

namespace vmm {

static PageTable kernel_pml4 = nullptr;
static uint64_t hhdm = 0;

static inline uint64_t read_cr3() {
    uint64_t val;
    asm volatile("mov %%cr3, %0" : "=r"(val));
    return val;
}

static inline void write_cr3(uint64_t val) {
    asm volatile("mov %0, %%cr3" : : "r"(val) : "memory");
}

static PageTable get_next_level(PageTable current_level, size_t index, bool allocate) {
    if (current_level[index] & PTE_PRESENT) {
        return (PageTable)((current_level[index] & PTE_ADDR_MASK) + hhdm);
    }
    
    if (!allocate) return nullptr;
    
    void* next_level_phys = pmm::alloc_page();
    if (!next_level_phys) return nullptr;
    
    // Calculate the physical address (alloc_page returns the virtual address in HHDM)
    uint64_t phys_addr = (uint64_t)next_level_phys - hhdm;
    
    // Set up the entry with PRESENT, WRITABLE, and USER flags.
    // We set USER flag here so that lower levels CAN have user access if needed.
    // Actual permission is AND'ed across all levels.
    current_level[index] = phys_addr | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    
    return (PageTable)next_level_phys;
}

void map_page(PageTable pml4, uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags) {
    size_t pml4_idx = (virtual_addr >> 39) & 0x1FF;
    size_t pdp_idx  = (virtual_addr >> 30) & 0x1FF;
    size_t pd_idx   = (virtual_addr >> 21) & 0x1FF;
    size_t pt_idx   = (virtual_addr >> 12) & 0x1FF;
    
    PageTable pdp = get_next_level(pml4, pml4_idx, true);
    if (!pdp) return;
    
    PageTable pd = get_next_level(pdp, pdp_idx, true);
    if (!pd) return;
    
    PageTable pt = get_next_level(pd, pd_idx, true);
    if (!pt) return;
    
    pt[pt_idx] = physical_addr | flags;
}

void init(uint64_t hhdm_offset) {
    hhdm = hhdm_offset;
    
    // Get the current PML4 physical address provided by Limine
    uint64_t current_cr3 = read_cr3();
    PageTable current_pml4 = (PageTable)(current_cr3 + hhdm);
    
    // Allocate a new page for our Kernel PML4
    kernel_pml4 = (PageTable)pmm::alloc_page();
    if (!kernel_pml4) {
        // Handle out of memory!
        for(;;) asm volatile("hlt");
    }
    
    // We only copy the higher half (indices 256 to 511)
    // The lower half will be left empty (0) for user space later.
    for (size_t i = 256; i < 512; i++) {
        kernel_pml4[i] = current_pml4[i];
    }
    
    // Switch to our new page table
    switch_pml4(kernel_pml4);
}

void switch_pml4(PageTable pml4) {
    uint64_t phys_addr = (uint64_t)pml4 - hhdm;
    write_cr3(phys_addr);
}

PageTable get_kernel_pml4() {
    return kernel_pml4;
}

}
