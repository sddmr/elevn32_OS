#pragma once

#include <stdint.h>

// 8x16 bitmap font — her karakter 8 piksel genişlik, 16 piksel yükseklik
// Her karakter 16 byte ile temsil edilir (her satır 1 byte = 8 bit = 8 piksel)

namespace font {

constexpr int CHAR_WIDTH = 8;
constexpr int CHAR_HEIGHT = 16;

// Font verisi: ASCII 32 (space) - 126 (~) arası = 95 karakter
extern const uint8_t font_data[95][16];

// Verilen ASCII karakterin font verisini döndürür
inline const uint8_t *get_glyph(char c) {
    if (c < 32 || c > 126) {
        return font_data[0]; // bilinmeyen → space
    }
    return font_data[c - 32];
}

} // namespace font
