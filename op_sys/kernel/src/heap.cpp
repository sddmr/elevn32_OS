#include "heap.h"
#include "string.h"

namespace heap {

struct Block {
    size_t size;
    bool free;
    Block *next;
};

static Block *head = nullptr;
static uint64_t heap_start = 0;
static uint64_t heap_size = 0;
static uint64_t heap_used = 0;

static const size_t BLOCK_SIZE = sizeof(Block);
static const size_t MIN_ALLOC = 16;

void init(uint64_t start, uint64_t size) {
    heap_start = start;
    heap_size = size;
    heap_used = 0;

    head = (Block *)start;
    head->size = size - BLOCK_SIZE;
    head->free = true;
    head->next = nullptr;
}

static Block *find_free(size_t size) {
    Block *current = head;
    while (current) {
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

static void split(Block *block, size_t size) {
    if (block->size >= size + BLOCK_SIZE + MIN_ALLOC) {
        Block *new_block = (Block *)((uint8_t *)block + BLOCK_SIZE + size);
        new_block->size = block->size - size - BLOCK_SIZE;
        new_block->free = true;
        new_block->next = block->next;
        block->size = size;
        block->next = new_block;
    }
}

void *kmalloc(size_t size) {
    if (size == 0) return nullptr;

    size = (size + 15) & ~15;

    Block *block = find_free(size);
    if (!block) return nullptr;

    split(block, size);
    block->free = false;
    heap_used += block->size + BLOCK_SIZE;

    return (void *)((uint8_t *)block + BLOCK_SIZE);
}

static void merge() {
    Block *current = head;
    while (current && current->next) {
        if (current->free && current->next->free) {
            current->size += current->next->size + BLOCK_SIZE;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

void kfree(void *ptr) {
    if (!ptr) return;

    Block *block = (Block *)((uint8_t *)ptr - BLOCK_SIZE);
    block->free = true;
    heap_used -= block->size + BLOCK_SIZE;

    merge();
}

uint64_t get_used() { return heap_used; }
uint64_t get_free() { return heap_size - heap_used; }

}

void *operator new(size_t size) { return heap::kmalloc(size); }
void *operator new[](size_t size) { return heap::kmalloc(size); }
void operator delete(void *ptr) noexcept { heap::kfree(ptr); }
void operator delete[](void *ptr) noexcept { heap::kfree(ptr); }
void operator delete(void *ptr, size_t) noexcept { heap::kfree(ptr); }
void operator delete[](void *ptr, size_t) noexcept { heap::kfree(ptr); }
