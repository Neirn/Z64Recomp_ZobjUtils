#include "helpers.h"

bool isMemEqual(const void *a, const void *b, size_t n) {
    const u8 *c = a;
    const u8 *d = b;

    for (size_t i = 0; i < n; i++) {
        if (c[i] != d[i]) {
            return false;
        }
    }

    return true;
}

u32 readU32(const u8 array[], u32 offset) {
    return (u32)(array[offset + 0]) << 24 |
           (u32)(array[offset + 1]) << 16 |
           (u32)(array[offset + 2]) << 8 |
           (u32)(array[offset + 3]);
}

void writeU32(u8 array[], u32 offset, u32 value) {
    array[offset + 0] = (value & 0xFF000000) >> 24;
    array[offset + 1] = (value & 0x00FF0000) >> 16;
    array[offset + 2] = (value & 0x0000FF00) >> 8;
    array[offset + 3] = (value & 0x000000FF);
}
