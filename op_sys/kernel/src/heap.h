#pragma once

#include <stdint.h>
#include <stddef.h>

namespace heap {

void init();
void *kmalloc(size_t size);
void kfree(void *ptr);
uint64_t get_used();
uint64_t get_free();

}

void *operator new(size_t size);
void *operator new[](size_t size);
void operator delete(void *ptr) noexcept;
void operator delete[](void *ptr) noexcept;
void operator delete(void *ptr, size_t) noexcept;
void operator delete[](void *ptr, size_t) noexcept;
