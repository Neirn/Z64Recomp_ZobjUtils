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
