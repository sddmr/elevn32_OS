#include "pmm.h"
#include "limine.h"
#include "string.h"

namespace pmm {

#define PAGE_SIZE 4096

static uint8_t *bitmap = nullptr;
static uint64_t bitmap_size = 0;
static uint64_t total_pages = 0;
static uint64_t used_pages = 0;
static uint64_t highest_addr = 0;
static uint64_t hhdm_offset = 0;

static inline void bitmap_set(uint64_t page) {
    bitmap[page / 8] |= (1 << (page % 8));
}

static inline void bitmap_clear(uint64_t page) {
    bitmap[page / 8] &= ~(1 << (page % 8));
}

static inline bool bitmap_test(uint64_t page) {
    return bitmap[page / 8] & (1 << (page % 8));
}

void init(void *memmap_resp, uint64_t hhdm) {
    hhdm_offset = hhdm;
    struct limine_memmap_response *response = (struct limine_memmap_response *)memmap_resp;

    for (uint64_t i = 0; i < response->entry_count; i++) {
        struct limine_memmap_entry *entry = response->entries[i];
        uint64_t top = entry->base + entry->length;
        if (top > highest_addr) {
            highest_addr = top;
        }
    }

    total_pages = highest_addr / PAGE_SIZE;
    bitmap_size = (total_pages + 7) / 8;

    for (uint64_t i = 0; i < response->entry_count; i++) {
        struct limine_memmap_entry *entry = response->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_size) {
            bitmap = (uint8_t *)(entry->base + hhdm_offset);
            memset(bitmap, 0xFF, bitmap_size);
            used_pages = total_pages;
            break;
        }
    }

    if (!bitmap) return;

    for (uint64_t i = 0; i < response->entry_count; i++) {
        struct limine_memmap_entry *entry = response->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t start_page = entry->base / PAGE_SIZE;
            uint64_t page_count = entry->length / PAGE_SIZE;
            for (uint64_t j = 0; j < page_count; j++) {
                bitmap_clear(start_page + j);
                used_pages--;
            }
        }
    }

    uint64_t bitmap_phys = (uint64_t)bitmap - hhdm_offset;
    uint64_t bitmap_pages = (bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint64_t bitmap_start_page = bitmap_phys / PAGE_SIZE;
    for (uint64_t i = 0; i < bitmap_pages; i++) {
        bitmap_set(bitmap_start_page + i);
        used_pages++;
    }
}

void *alloc_page() {
    for (uint64_t i = 0; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            used_pages++;
            void *page = (void *)(i * PAGE_SIZE + hhdm_offset);
            memset(page, 0, PAGE_SIZE);
            return page;
        }
    }
    return nullptr;
}

void free_page(void *addr) {
    uint64_t page = ((uint64_t)addr - hhdm_offset) / PAGE_SIZE;
    if (page < total_pages && bitmap_test(page)) {
        bitmap_clear(page);
        used_pages--;
    }
}

uint64_t get_total_memory() { return total_pages * PAGE_SIZE; }
uint64_t get_used_memory() { return used_pages * PAGE_SIZE; }
uint64_t get_free_memory() { return (total_pages - used_pages) * PAGE_SIZE; }

}
