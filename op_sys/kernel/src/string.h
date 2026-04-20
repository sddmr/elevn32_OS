#pragma once

#include <stdint.h>
#include <stddef.h>

// Temel string fonksiyonları — standart kütüphane olmadan kendimiz yazıyoruz

extern "C" {

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *dest, int val, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

size_t strlen(const char *str);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);

} // extern "C"

// Sayıyı string'e çeviren yardımcı fonksiyonlar
void int_to_str(int64_t value, char *buffer, int base = 10);
void uint_to_str(uint64_t value, char *buffer, int base = 10);
